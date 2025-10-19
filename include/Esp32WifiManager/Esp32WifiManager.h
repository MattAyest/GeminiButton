/*
    Description: wifi manager for esp32 C projects
    Creator: Matthew Ayestaran
    date:19/10/2025
*/

#ifdef ESP32WifiManager.h
#define ESP32WifiManager.h

#include "freertos/event_groups.h"

esp_err_t wifi_manager_wait_for_connection(TickType_t xTicksToWait);


#endif



