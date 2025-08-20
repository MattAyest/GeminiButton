#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "driver/gpio.h"
#include "esp_crt_bundle.h"


// Configuration
// These values are set by build flags in platformio.ini or via menuconfig
#define GEMINI_API_KEY CONFIG_GEMINI_API_KEY
#define WIFI_SSID      CONFIG_WIFI_SSID
#define WIFI_PASS      CONFIG_WIFI_PASSWORD
#define WIFI_MAXIMUM_RETRY  5
#define BUTTON_GPIO         0 // GPIO 0 is often the "BOOT" button on dev kits
#define GEMINI_TASK_STACK_SIZE 10240
#define GEMINI_API_KEY "AIzaSyA8g0_zO89HndS1Q9eKj9Gy5BX23noIqmQ"
#define WIFI_SSID  "Pixel_4407"
#define WIFI_PASS  "wagwanjimbo"

static const char *TAG = "gemini_api_call";

#define MODEL_NAME "gemini-1.5-flash"
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* FreeRTOS semaphore to trigger the Gemini API call */
static SemaphoreHandle_t s_gemini_semaphore;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static int s_retry_num = 0;
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

static const char* const questions[] = {
    "What is the capital of France?",
    "What is the tallest mountain in the world?",
    "Who wrote the play Romeo and Juliet?",
    "What is the chemical symbol for water?"
};
static int question_index = 0;

static void gemini_api_task(void *pvParameters);

// Helper function to parse the JSON response and extract the model's text.
// Returns a newly allocated string that must be freed by the caller.
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
        ESP_LOGE(TAG, "JSON response missing 'candidates' array.");
        goto end;
    }

    const cJSON *first_candidate = cJSON_GetArrayItem(candidates, 0);
    if (first_candidate == NULL) {
        ESP_LOGE(TAG, "Could not get first candidate");
        goto end;
    }
    const cJSON *content = cJSON_GetObjectItem(first_candidate, "content");
    if (!content) {
        ESP_LOGE(TAG, "JSON response missing 'content' object.");
        goto end;
    }


    const cJSON *parts = cJSON_GetObjectItem(content, "parts");
    if (!cJSON_IsArray(parts) || cJSON_GetArraySize(parts) == 0) {
        ESP_LOGE(TAG, "JSON response missing 'parts' array.");
        goto end;
    }

    const cJSON *first_part = cJSON_GetArrayItem(parts, 0);
    const cJSON *text = cJSON_GetObjectItem(first_part, "text");
    if (!cJSON_IsString(text) || (text->valuestring == NULL)) {
        ESP_LOGE(TAG, "JSON response missing 'text' string.");
        goto end;
    }

    result_text = strdup(text->valuestring);
    if (result_text == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for result text.");
    }

end:
    cJSON_Delete(root);
    return result_text;
}

static char* create_gemini_json_payload(const char* question) {
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "Failed to create cJSON root object.");
        return NULL;
    }

    cJSON *contents = cJSON_AddArrayToObject(root, "contents");
    if (!contents) goto error;

    cJSON *content_item = cJSON_CreateObject();
    if (!content_item) goto error;
    cJSON_AddItemToArray(contents, content_item);

    cJSON *parts = cJSON_AddArrayToObject(content_item, "parts");
    if (!parts) goto error;

    cJSON *part_item = cJSON_CreateObject();
    if (!part_item) goto error;
    cJSON_AddItemToArray(parts, part_item);

    cJSON_AddStringToObject(part_item, "text", question);

    cJSON *safety_settings = cJSON_AddArrayToObject(root, "safetySettings");
    if (!safety_settings) goto error;

    const char* categories[] = {
        "HARM_CATEGORY_DANGEROUS_CONTENT",
        "HARM_CATEGORY_HATE_SPEECH",
        "HARM_CATEGORY_HARASSMENT",
        "HARM_CATEGORY_SEXUALLY_EXPLICIT"
    };

    for (int i = 0; i < sizeof(categories)/sizeof(categories[0]); i++) {
        cJSON *setting_item = cJSON_CreateObject();
        if (!setting_item) goto error;
        cJSON_AddStringToObject(setting_item, "category", categories[i]);
        cJSON_AddStringToObject(setting_item, "threshold", "BLOCK_NONE");
        cJSON_AddItemToArray(safety_settings, setting_item);
    }

    cJSON *generation_config = cJSON_CreateObject();
    if (!generation_config) goto error;

    cJSON_AddNumberToObject(generation_config, "temperature", 0.9);
    cJSON_AddNumberToObject(generation_config, "topK", 1);
    cJSON_AddNumberToObject(generation_config, "topP", 1);
    cJSON_AddNumberToObject(generation_config, "maxOutputTokens", 8192);

    if (!cJSON_AddItemToObject(root, "generationConfig", generation_config)) {
        cJSON_Delete(generation_config);
        goto error;
    }

    char *json_string = cJSON_Print(root);
    cJSON_Delete(root);
    return json_string;

error:
    ESP_LOGE(TAG, "Failed to create Gemini JSON payload due to memory allocation failure.");
    cJSON_Delete(root);
    return NULL;
}

// Function to make the Gemini API call
esp_err_t make_gemini_api_call(const char *question, char **response_data) {
    *response_data = NULL;
    esp_err_t err = ESP_FAIL;
    char *post_data = NULL;
    esp_http_client_handle_t client = NULL;

    char gemini_url[256];
    snprintf(gemini_url, sizeof(gemini_url), "https://generativeai.googleapis.com/v1beta/models/%s:generateContent?key=%s", MODEL_NAME, GEMINI_API_KEY);
    
    ESP_LOGI(TAG, "Requesting URL for model: %s", MODEL_NAME);

    const esp_http_client_config_t config = {
        .url = gemini_url,
        .method = HTTP_METHOD_POST,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 20000,
        .buffer_size = 4096,
        .buffer_size_tx = 2048,
    };
    client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    post_data = create_gemini_json_payload(question);
    if (post_data == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON post data.");
        err = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");

    if ((err = esp_http_client_perform(client)) != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
        goto cleanup;
    }

    int status_code = esp_http_client_get_status_code(client);
    int64_t content_length = esp_http_client_get_content_length(client);
    ESP_LOGI(TAG, "HTTP Status = %d, content_length = %lld", status_code, content_length);

    if (status_code != 200) {
        ESP_LOGE(TAG, "Request failed with status code %d", status_code);
        char error_buf[512];
        int error_read_len = esp_http_client_read_response(client, error_buf, sizeof(error_buf) - 1);
        if (error_read_len > 0) {
            error_buf[error_read_len] = '\0';
            ESP_LOGE(TAG, "Server error response: %s", error_buf);
        }
        err = ESP_FAIL;
        goto cleanup;
    }

    if (content_length <= 0) {
        ESP_LOGW(TAG, "Content length is zero or unknown, cannot read response body.");
        err = ESP_OK;
        goto cleanup;
    }

    *response_data = (char *)malloc(content_length + 1);
    if (*response_data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for response data");
        err = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    int read_len = esp_http_client_read_response(client, *response_data, content_length);
    if (read_len < 0) {
        ESP_LOGE(TAG, "Error reading response");
        err = ESP_FAIL;
    } else {
        (*response_data)[read_len] = '\0';
        err = ESP_OK;
    }

    if (err != ESP_OK) {
        free(*response_data);
        *response_data = NULL;
    }

cleanup:
    free(post_data);
    if (client) {
        esp_http_client_cleanup(client);
    }
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

    ESP_LOGI(TAG, "wifi_init_sta finished. Waiting for connection...");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID:%s", WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to SSID:%s", WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED WIFI EVENT");
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
        ESP_LOGW(TAG, "Disconnected from AP, trying to reconnect...");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void IRAM_ATTR button_isr_handler(void* arg)
{
    gpio_intr_disable(BUTTON_GPIO);
    xSemaphoreGiveFromISR(s_gemini_semaphore, NULL);
}

static void peripherals_init(void)
{
    s_gemini_semaphore = xSemaphoreCreateBinary();

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);
}

static void gemini_api_task(void *pvParameters) {
    while(1) {
        if (xSemaphoreTake(s_gemini_semaphore, portMAX_DELAY) == pdTRUE) {
            EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
            if (!(bits & WIFI_CONNECTED_BIT)) {
                ESP_LOGE(TAG, "Wi-Fi not connected. Aborting API call.");
                vTaskDelay(pdMS_TO_TICKS(500));
                gpio_intr_enable(BUTTON_GPIO);
                continue;
            }

            const char* current_question = questions[question_index];
            ESP_LOGI(TAG, "Button pressed! Asking: \"%s\"", current_question);
            ESP_LOGI(TAG, "Free heap before API call: %lu bytes", (unsigned long)esp_get_free_heap_size());
            
            char *raw_response = NULL;
            esp_err_t err = make_gemini_api_call(current_question, &raw_response);

            if (err == ESP_OK && raw_response != NULL) {
                char* gemini_answer = parse_gemini_response(raw_response);
                if (gemini_answer != NULL) {
                    ESP_LOGI(TAG, "Gemini's Answer: %s", gemini_answer);
                    free(gemini_answer);
                }
                free(raw_response);
            } else {
                ESP_LOGE(TAG, "API call failed!");
            }
            ESP_LOGI(TAG, "Finished Gemini API call. Ready for next button press.");

            question_index = (question_index + 1) % (sizeof(questions) / sizeof(questions[0]));

            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_intr_enable(BUTTON_GPIO);
        }
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

    peripherals_init();

    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
    if (bits & WIFI_CONNECTED_BIT) {
        xTaskCreate(gemini_api_task, "gemini_task", GEMINI_TASK_STACK_SIZE, NULL, 5, NULL);
        ESP_LOGI(TAG, "Initialization complete. Press the button to ask Gemini a question.");
    } else {
        ESP_LOGE(TAG, "Wi-Fi not connected. Cannot start Gemini task.");
    }
}
