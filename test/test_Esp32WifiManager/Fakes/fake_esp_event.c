/*
 *description: code for faking the esp_event.c library for testing
 *creator:Matthew Ayestaran
 *Date:10/25/25
 * */

#include "esp_event.h"
#include <string.h>

esp_event_handler_t g_registered_wifi_handler = NULL;
esp_event_handler_t g_registered_ip_handler = NULL;

esp_err_t esp_event_handler_register(esp_event_base_t event_base,
                                     int32_t event_id,
                                     esp_event_handler_t event_handler,
                                     void *arg) {
  // Save the function pointer based on the event base
  if (strcmp(event_base, WIFI_EVENT) == 0) {
    g_registered_wifi_handler = event_handler;
  } else if (strcmp(event_base, IP_EVENT) == 0) {
    g_registered_ip_handler = event_handler;
  }
  return ESP_OK; // Always succeed by default
}

FAKE_VALUE_FUNC(esp_err_t, esp_event_loop_create_default);
