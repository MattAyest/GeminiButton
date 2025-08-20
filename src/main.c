#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include "esp_crt_bundle.h"

// =================================================================================
// Configuration
// These values are set by build flags in platformio.ini
#define GEMINI_API_KEY CONFIG_GEMINI_API_KEY
#define WIFI_SSID      CONFIG_WIFI_SSID
#define WIFI_PASS      CONFIG_WIFI_PASSWORD
// =================================================================================

#define WIFI_MAXIMUM_RETRY   5
#define GEMINI_TASK_STACK_SIZE 10240
#define API_CALL_MAX_RETRIES 3
#define SERIAL_BUFFER_SIZE   256


static const char *TAG = "gemini_api_call";

#define MODEL_NAME "gemini-2.0-flash"
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static int s_retry_num = 0;
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

static void gemini_api_task(void *pvParameters);

// Helper function to parse the JSON response and extract the model's text.
char* parse_gemini_response(const char* json_string)
{
    char* result_text = NULL;
    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return NULL;
    }

    const cJSON *candidates = cJSON_GetObjectItem(root, "candidates");
    if (!cJSON_IsArray(candidates) || cJSON_GetArraySize(candidates) == 0) {
        const cJSON *promptFeedback = cJSON_GetObjectItem(root, "promptFeedback");
        if (promptFeedback) {
            const cJSON *blockReason = cJSON_GetObjectItem(promptFeedback, "blockReason");
            if (cJSON_IsString(blockReason)) {
                ESP_LOGE(TAG, "Request blocked, reason: %s", blockReason->valuestring);
            }
        } else {
            ESP_LOGE(TAG, "JSON response missing 'candidates' array.");
        }
        goto end;
    }

    const cJSON *first_candidate = cJSON_GetArrayItem(candidates, 0);
    const cJSON *content = cJSON_GetObjectItem(first_candidate, "content");
    const cJSON *parts = cJSON_GetObjectItem(content, "parts");
    const cJSON *first_part = cJSON_GetArrayItem(parts, 0);
    const cJSON *text = cJSON_GetObjectItem(first_part, "text");

    if (cJSON_IsString(text) && text->valuestring != NULL) {
        result_text = strdup(text->valuestring);
    } else {
        ESP_LOGE(TAG, "JSON response missing 'text' string.");
    }

end:
    cJSON_Delete(root);
    return result_text;
}

// Creates the correct JSON payload structure for the Gemini API.
static char* create_gemini_json_payload(const char* question) {
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "Failed to create cJSON root object.");
        return NULL;
    }

    cJSON *contents = cJSON_AddArrayToObject(root, "contents");
    if (!contents) goto error;

    cJSON *content_item = cJSON_CreateObject();
    cJSON_AddItemToArray(contents, content_item);

    cJSON *parts = cJSON_AddArrayToObject(content_item, "parts");
    cJSON *part_item = cJSON_CreateObject();
    cJSON_AddItemToArray(parts, part_item);
    cJSON_AddStringToObject(part_item, "text", question);

    cJSON *safety_settings = cJSON_AddArrayToObject(root, "safetySettings");
    const char* categories[] = {
        "HARM_CATEGORY_DANGEROUS_CONTENT", "HARM_CATEGORY_HATE_SPEECH",
        "HARM_CATEGORY_HARASSMENT", "HARM_CATEGORY_SEXUALLY_EXPLICIT"
    };
    for (int i = 0; i < sizeof(categories)/sizeof(categories[0]); i++) {
        cJSON *setting_item = cJSON_CreateObject();
        cJSON_AddStringToObject(setting_item, "category", categories[i]);
        cJSON_AddStringToObject(setting_item, "threshold", "BLOCK_NONE");
        cJSON_AddItemToArray(safety_settings, setting_item);
    }

    cJSON *generation_config = cJSON_CreateObject();
    cJSON_AddNumberToObject(generation_config, "temperature", 0.9);
    cJSON_AddNumberToObject(generation_config, "topK", 1);
    cJSON_AddNumberToObject(generation_config, "topP", 1);
    cJSON_AddNumberToObject(generation_config, "maxOutputTokens", 8192);
    cJSON_AddItemToObject(root, "generationConfig", generation_config);

    char *json_string = cJSON_Print(root);
    cJSON_Delete(root);
    return json_string;

error:
    ESP_LOGE(TAG, "Failed to create Gemini JSON payload.");
    cJSON_Delete(root);
    return NULL;
}

// Using a struct to manage the response buffer for the event handler
typedef struct {
    char *buffer;
    int buffer_size;
    int data_len;
} http_response_buffer_t;

// Robust HTTP event handler to correctly process chunked responses
esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    http_response_buffer_t *response_buffer = (http_response_buffer_t *)evt->user_data;

    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (response_buffer->data_len + evt->data_len + 1 > response_buffer->buffer_size) {
                int required_size = response_buffer->data_len + evt->data_len + 1;
                int new_size = response_buffer->buffer_size;
                while (new_size < required_size) {
                    new_size *= 2;
                }
                char *new_buffer = realloc(response_buffer->buffer, new_size);
                if (new_buffer == NULL) {
                    ESP_LOGE(TAG, "Failed to reallocate memory");
                    return ESP_FAIL;
                }
                response_buffer->buffer = new_buffer;
                response_buffer->buffer_size = new_size;
            }
            memcpy(response_buffer->buffer + response_buffer->data_len, evt->data, evt->data_len);
            response_buffer->data_len += evt->data_len;
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (response_buffer->buffer != NULL) {
                response_buffer->buffer[response_buffer->data_len] = '\0';
            }
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

// Function to make the Gemini API call
esp_err_t make_gemini_api_call(const char *question, char **response_data) {
    *response_data = NULL;
    char *post_data = create_gemini_json_payload(question);
    if (post_data == NULL) {
        return ESP_ERR_NO_MEM;
    }

    http_response_buffer_t response_buffer = {0};
    response_buffer.buffer = malloc(2048);
    if (response_buffer.buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for response buffer");
        free(post_data);
        return ESP_ERR_NO_MEM;
    }
    response_buffer.buffer_size = 2048;

    char gemini_url[256];
    snprintf(gemini_url, sizeof(gemini_url), "https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent", MODEL_NAME);
    
    esp_http_client_config_t config = {
        .url = gemini_url,
        .method = HTTP_METHOD_POST,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 30000,
        .user_data = &response_buffer,
        .event_handler = http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "x-goog-api-key", GEMINI_API_KEY);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Status = %d", status_code);
        if (status_code == 200) {
            *response_data = response_buffer.buffer;
        } else {
            ESP_LOGE(TAG, "Server returned error status: %d", status_code);
            ESP_LOGE(TAG, "Response: %s", response_buffer.buffer);
            free(response_buffer.buffer);
            err = ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        free(response_buffer.buffer);
    }
    
    esp_http_client_cleanup(client);
    free(post_data);
    return err;
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
           .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID: %s", WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to SSID: %s", WIFI_SSID);
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// **REMOVED:** Button-related functions are no longer needed.

static void gemini_api_task(void *pvParameters) {
    char line_buffer[SERIAL_BUFFER_SIZE];
    int index = 0;

    // Give a small delay to ensure the serial monitor is ready
    vTaskDelay(pdMS_TO_TICKS(1000));
    printf("Enter your question and press Enter:\n");

    while(1) {
        // Read characters from stdin (serial monitor)
        int c = fgetc(stdin);
        if (c != EOF) {
            // Echo character back to the monitor
            putchar(c);
            
            if (c == '\n' || c == '\r') {
                // End of line
                line_buffer[index] = '\0';
                
                if (index > 0) {
                    ESP_LOGI(TAG, "Question received: \"%s\"", line_buffer);
                    
                    char *raw_response = NULL;
                    esp_err_t err = ESP_FAIL;

                    // Retry logic for the API call
                    for (int retries = 0; retries < API_CALL_MAX_RETRIES; retries++) {
                        err = make_gemini_api_call(line_buffer, &raw_response);
                        if (err == ESP_OK) {
                            break; 
                        }
                        ESP_LOGW(TAG, "API call failed, retrying in %d seconds...", 1 << retries);
                        vTaskDelay(pdMS_TO_TICKS(1000 * (1 << retries))); 
                    }

                    if (err == ESP_OK && raw_response != NULL) {
                        ESP_LOGI(TAG, "Raw response received: [%s]", raw_response);
                        char *json_start = strchr(raw_response, '{');
                        if (json_start) {
                            char* gemini_answer = parse_gemini_response(json_start);
                            if (gemini_answer != NULL) {
                                printf("\nGemini's Answer: %s\n", gemini_answer);
                                free(gemini_answer);
                            }
                        } else {
                            ESP_LOGE(TAG, "No JSON object found in response");
                        }
                        free(raw_response);
                    } else {
                        ESP_LOGE(TAG, "API call failed after %d retries.", API_CALL_MAX_RETRIES);
                    }
                }
                
                // Reset for next question
                index = 0;
                printf("\nEnter your question and press Enter:\n");

            } else if (index < (SERIAL_BUFFER_SIZE - 1)) {
                // Add character to buffer
                line_buffer[index++] = c;
            }
        }
        // Small delay to prevent task from spinning
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();

    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
    if (bits & WIFI_CONNECTED_BIT) {
        xTaskCreate(gemini_api_task, "gemini_task", GEMINI_TASK_STACK_SIZE, NULL, 5, NULL);
        ESP_LOGI(TAG, "Initialization complete. Ready for serial input.");
    } else {
        ESP_LOGE(TAG, "Wi-Fi not connected. Cannot start Gemini task.");
    }
}
