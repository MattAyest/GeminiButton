/*
    Description: wifi manager for esp32 C projects
    Creator: Matthew Ayestaran
    date:19/10/2025
*/

// #ifdef ESP32WIFIMANAGER_H
#define ESP32WIFIMANAGER_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

void wifi_manager_init_station(const wifi_manager_config_t *config);
esp_err_t wifi_manager_wait_for_connection(TickType_t xTicksToWait);

// #endif
