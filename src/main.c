#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
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

static const char *TAG = "gemini_chat_grounding";

#define MODEL_NAME "gemini-1.5-pro"
static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static int s_retry_num = 0;

// Global state for chat
cJSON *chat_history = NULL;
char *cached_content_name = NULL; // Stores the "cachedContents/..." token

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void gemini_api_task(void *pvParameters);

// ======================= Chat History Management =======================

void add_message_to_history(const char *role, const char *text) {
    if (!chat_history) {
        chat_history = cJSON_CreateArray();
    }
    cJSON *message = cJSON_CreateObject();
    cJSON_AddStringToObject(message, "role", role);
    cJSON *parts = cJSON_AddArrayToObject(message, "parts");
    cJSON *part_item = cJSON_CreateObject();
    cJSON_AddStringToObject(part_item, "text", text);
    cJSON_AddItemToArray(parts, part_item);
    cJSON_AddItemToArray(chat_history, message);
}

void clear_chat_history() {
    if (chat_history) {
        cJSON_Delete(chat_history);
    }
    if (cached_content_name) {
        free(cached_content_name);
        cached_content_name = NULL;
    }
    chat_history = cJSON_CreateArray();
    ESP_LOGI(TAG, "Chat history and cache cleared.");
}

// ======================= API and JSON Functions =======================

typedef struct {
    char* text;
    char* cache_name;
} parsed_response_t;


parsed_response_t parse_gemini_response(const char* json_string) {
    parsed_response_t response = { .text = NULL, .cache_name = NULL };
    cJSON *root = cJSON_Parse(json_string);
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return response;
    }

    const cJSON *cached_content = cJSON_GetObjectItem(root, "cachedContent");
    if (cJSON_IsString(cached_content) && cached_content->valuestring != NULL) {
        response.cache_name = strdup(cached_content->valuestring);
    }

    const cJSON *candidates = cJSON_GetObjectItem(root, "candidates");
    if (!cJSON_IsArray(candidates) || cJSON_GetArraySize(candidates) == 0) {
        goto end;
    }

    const cJSON *first_candidate = cJSON_GetArrayItem(candidates, 0);
    const cJSON *content = cJSON_GetObjectItem(first_candidate, "content");
    const cJSON *parts = cJSON_GetObjectItem(content, "parts");
    const cJSON *first_part = cJSON_GetArrayItem(parts, 0);
    const cJSON *text = cJSON_GetObjectItem(first_part, "text");

    if (cJSON_IsString(text) && text->valuestring != NULL) {
        response.text = strdup(text->valuestring);
    }

end:
    cJSON_Delete(root);
    return response;
}

static char* create_gemini_json_payload(const char* new_question) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    if (cached_content_name) {
        cJSON *contents = cJSON_AddArrayToObject(root, "contents");
        cJSON *content_item = cJSON_CreateObject();
        cJSON_AddItemToArray(contents, content_item);
        cJSON *parts = cJSON_AddArrayToObject(content_item, "parts");
        cJSON *part_item = cJSON_CreateObject();
        cJSON_AddItemToArray(parts, part_item);
        cJSON_AddStringToObject(part_item, "text", new_question);
        
        cJSON_AddStringToObject(root, "cachedContent", cached_content_name);
    } else {
        add_message_to_history("user", new_question);
        cJSON_AddItemToObject(root, "contents", cJSON_Duplicate(chat_history, 1));
    }

    // **FIX:** Add tools section to enable grounding with Google Search
    cJSON *tools = cJSON_AddArrayToObject(root, "tools");
    cJSON *tool_item = cJSON_CreateObject();
    cJSON_CreateObject(); // googleSearchRetrieval object
    cJSON_AddItemToObject(tool_item, "googleSearchRetrieval", cJSON_CreateObject());
    cJSON_AddItemToArray(tools, tool_item);


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
    cJSON_AddItemToObject(root, "generationConfig", generation_config);

    char *json_string = cJSON_Print(root);
    cJSON_Delete(root);
    return json_string;
}

typedef struct {
    char *buffer;
    int buffer_size;
    int data_len;
} http_response_buffer_t;

esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    http_response_buffer_t *response_buffer = (http_response_buffer_t *)evt->user_data;
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (response_buffer->buffer_size < response_buffer->data_len + evt->data_len + 1) {
                int new_size = response_buffer->buffer_size * 2;
                char *new_buffer = realloc(response_buffer->buffer, new_size);
                if (new_buffer == NULL) { return ESP_FAIL; }
                response_buffer->buffer = new_buffer;
                response_buffer->buffer_size = new_size;
            }
            memcpy(response_buffer->buffer + response_buffer->data_len, evt->data, evt->data_len);
            response_buffer->data_len += evt->data_len;
            break;
        case HTTP_EVENT_ON_FINISH:
            if (response_buffer->buffer != NULL) {
                response_buffer->buffer[response_buffer->data_len] = '\0';
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t make_gemini_api_call(const char *question, char **response_data) {
    *response_data = NULL;
    char *post_data = create_gemini_json_payload(question);
    if (post_data == NULL) return ESP_ERR_NO_MEM;

    http_response_buffer_t response_buffer = {0};
    response_buffer.buffer = malloc(2048);
    if (response_buffer.buffer == NULL) {
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
        if (esp_http_client_get_status_code(client) == 200) {
            *response_data = response_buffer.buffer;
        } else {
            ESP_LOGE(TAG, "HTTP Status = %d", esp_http_client_get_status_code(client));
            ESP_LOGE(TAG, "Response: %s", response_buffer.buffer);
            free(response_buffer.buffer);
            err = ESP_FAIL;
        }
    } else {
        free(response_buffer.buffer);
    }
    
    esp_http_client_cleanup(client);
    free(post_data);
    return err;
}

// ======================= Main Task and System Init =======================

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
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = { .sta = { .ssid = WIFI_SSID, .password = WIFI_PASS } };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID: %s", WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "Failed to connect to SSID: %s", WIFI_SSID);
    }
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void gemini_api_task(void *pvParameters) {
    char line_buffer[SERIAL_BUFFER_SIZE];
    int index = 0;

    printf("\nEnter your question and press Enter.\nType 'new' to start a new conversation.\n");

    while(1) {
        int c = fgetc(stdin);
        if (c != EOF) {
            putchar(c);
            
            if (c == '\n' || c == '\r') {
                line_buffer[index] = '\0';
                
                if (index > 0) {
                    if (strcmp(line_buffer, "new") == 0) {
                        clear_chat_history();
                    } else {
                        char *raw_response = NULL;
                        esp_err_t err = make_gemini_api_call(line_buffer, &raw_response);

                        if (err == ESP_OK && raw_response != NULL) {
                            parsed_response_t parsed = parse_gemini_response(raw_response);
                            
                            if (parsed.text) {
                                printf("\nGemini: %s\n", parsed.text);
                                add_message_to_history("user", line_buffer);
                                add_message_to_history("model", parsed.text);
                                free(parsed.text);
                            }

                            if (parsed.cache_name) {
                                if (cached_content_name) free(cached_content_name);
                                cached_content_name = parsed.cache_name;
                                ESP_LOGI(TAG, "Updated cache token: %s", cached_content_name);
                            }
                            free(raw_response);
                        } else {
                            ESP_LOGE(TAG, "API call failed. Cache may have expired. Try starting a new conversation.");
                        }
                    }
                }
                
                index = 0;
                printf("\nEnter your question and press Enter.\n");

            } else if (index < (SERIAL_BUFFER_SIZE - 1)) {
                line_buffer[index++] = c;
            }
        }
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
    
    // Initialize a new, empty chat history on boot
    chat_history = cJSON_CreateArray();

    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
    if (bits & WIFI_CONNECTED_BIT) {
        xTaskCreate(gemini_api_task, "gemini_task", GEMINI_TASK_STACK_SIZE, NULL, 5, NULL);
    } else {
        ESP_LOGE(TAG, "Wi-Fi not connected. Cannot start Gemini task.");
    }
}
