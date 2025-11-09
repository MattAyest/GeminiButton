
#ifndef FAKE_ESP_WIFI_H
#define FAKE_ESP_WIFI_H

#include "fake_esp_common.h" // <-- Rule 3: Use the dictionary
#include "fff.h"

#define WIFI_EVENT "WIFI_EVENT"
#define WIFI_EVENT_STA_START 1
#define WIFI_EVENT_STA_DISCONNECTED 2
#define ESP_EVENT_ANY_ID 0

typedef enum { WIFI_MODE_STA } wifi_mode_t;
#define ESP_IF_WIFI_STA 0

typedef struct {
  const char *ssid;
  const char *password;
} wifi_sta_config_t;

typedef union {
  wifi_sta_config_t sta;
} wifi_config_t;

DECLARE_FAKE_VALUE_FUNC(esp_err_t, esp_wifi_init, const wifi_init_config_t *);
DECLARE_FAKE_VALUE_FUNC(esp_err_t, esp_wifi_set_mode, wifi_mode_t);
DECLARE_FAKE_VALUE_FUNC(esp_err_t, esp_wifi_set_config, esp_interface_t,
                        wifi_config_t *);
DECLARE_FAKE_VALUE_FUNC(esp_err_t, esp_wifi_start);
DECLARE_FAKE_VALUE_FUNC(esp_err_t, esp_wifi_connect);

#endif
