/*
    Gemini Button code. it should make a esp32 button that casn be pressed and an input can be sent to gemini with an expected output. This will me fed into a Text to speach api.
    written by: Matthew Ayestan
    Date:22/08/2025

*/

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
//Driver for I2S for Mic Mic used is SPH0645
#include "driver/i2s.h"

//Header calls
#include "GeminiAPI.h"
#include "I2S_Audio_Controller.h"

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

//I2S definitions
#define Sample_Rate 16000

//GPIO pin declarations
#define I2S_LRCL_Pin 5
#define I2S_Blck_Pin 6
#define I2S_Dout_Pin 7 

// Global state for chat
char *cached_content_name = NULL; // Stores the "cachedContents/..." token

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
static void gemini_api_task(void *pvParameters);

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

    printf("\nEnter your question and press Enter.\n");

    while(1) {
        int c = fgetc(stdin);
        if (c != EOF) {
            putchar(c);
            
            if (c == '\n' || c == '\r') {
                line_buffer[index] = '\0';
                
                if (index > 0) {
                    char *raw_response = NULL;
                    esp_err_t err = make_gemini_api_call(line_buffer, &raw_response);

                    if (err == ESP_OK && raw_response != NULL) {
                        parsed_response_t parsed = parse_gemini_response(raw_response);
                        
                        if (parsed.text) {
                            printf("\nGemini: %s\n", parsed.text);
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

    EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
    if (bits & WIFI_CONNECTED_BIT) {
        xTaskCreate(gemini_api_task, "gemini_task", GEMINI_TASK_STACK_SIZE, NULL, 5, NULL);
    } else {
        ESP_LOGE(TAG, "Wi-Fi not connected. Cannot start Gemini task.");
    }
}
