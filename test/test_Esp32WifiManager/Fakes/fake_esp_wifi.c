/*
 * description: File for recreating the fake_esp_wifi library for testing
 * creator:Matthew Ayestaran
 * date:10/25/2025
 */
#include "esp_wifi.h" // Include its own header

FAKE_VALUE_FUNC(esp_err_t, esp_wifi_init, const wifi_init_config_t *);
FAKE_VALUE_FUNC(esp_err_t, esp_wifi_set_mode, wifi_mode_t);
FAKE_VALUE_FUNC(esp_err_t, esp_wifi_set_config, esp_interface_t,
                wifi_config_t *);
FAKE_VALUE_FUNC(esp_err_t, esp_wifi_start);
FAKE_VALUE_FUNC(esp_err_t, esp_wifi_connect);
