#ifndef FAKE_ESP_COMMON_H
#define FAKE_ESP_COMMON_H

#include <stdint.h> // For int32_t, uint32_t, etc.

/* --- Common ESP-IDF Types --- */
typedef int32_t esp_err_t;
typedef const char *esp_event_base_t;
typedef void *esp_netif_t;
typedef void *wifi_init_config_t;
typedef int esp_interface_t;

/* --- Common FreeRTOS Types --- */
typedef uint32_t TickType_t;
typedef void *TaskHandle_t;
typedef uint8_t BaseType_t;
typedef void *EventGroupHandle_t;
typedef TickType_t EventBits_t;

/* --- Common Error Codes --- */
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_TIMEOUT 0x107
#define ESP_ERR_NVS_NO_FREE_PAGES 0x1101
#define ESP_ERR_NVS_NEW_VERSION_FOUND 0x1102

/* --- Common #defines --- */
#define pdFALSE ((BaseType_t)0)
#define pdTRUE ((BaseType_t)1)
#define portMAX_DELAY ((TickType_t)0xFFFFFFFFUL)
#define BIT0 (1 << 0)
#define BIT1 (1 << 1)
#define ESP_EVENT_ANY_ID 0
#define WIFI_INIT_CONFIG_DEFAULT() NULL

#endif // FAKE_ESP_COMMON_H
