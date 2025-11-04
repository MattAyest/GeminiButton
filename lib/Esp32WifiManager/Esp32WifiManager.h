/*
    Description: wifi manager for esp32 C projects
    Creator: Matthew Ayestaran
    date:19/10/2025
*/

#ifndef ESP32WIFIMANAGER_H
#define ESP32WIFIMANAGER_H

#include <stdint.h>

typedef struct {
  const char *ssid;
  const char *password;
} wifi_manager_config_t;

typedef int32_t esp_err_t;
typedef uint32_t TickType_t;

void wifi_manager_init_station(const wifi_manager_config_t *config);
esp_err_t wifi_manager_wait_for_connection(TickType_t xTicksToWait);

#endif
