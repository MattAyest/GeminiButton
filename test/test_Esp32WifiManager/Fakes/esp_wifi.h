#ifndef FAKE_ESP_WIFI_H
#define FAKE_ESP_WIFI_H
#define WIFI_EVENT "WIFI_EVENT" // Use a string for the base
#define WIFI_EVENT_STA_START 1
#define WIFI_EVENT_STA_DISCONNECTED 2
#define ESP_EVENT_ANY_ID 0 // This is also used by your code
#include <stdint.h>        // For int32_t

// --- Add fake types your code needs ---
typedef int32_t esp_err_t; // You should have this in a central fake header
typedef void *wifi_init_config_t;
typedef int esp_interface_t;

// --- Fake enums and defines ---
typedef enum { WIFI_MODE_STA } wifi_mode_t;

#define ESP_IF_WIFI_STA 0
#define WIFI_INIT_CONFIG_DEFAULT() NULL // Fake default config

// --- Fake struct definitions ---
// Your code uses wifi_config_t and its .sta member
typedef struct {
  const char *ssid;
  const char *password;
} wifi_sta_config_t;

typedef union {
  wifi_sta_config_t sta;
} wifi_config_t;

// --- Add fake function prototypes ---
esp_err_t esp_wifi_init(const wifi_init_config_t *config);
esp_err_t esp_wifi_set_mode(wifi_mode_t mode);
esp_err_t esp_wifi_set_config(esp_interface_t interface, wifi_config_t *conf);
esp_err_t esp_wifi_start(void);
esp_err_t esp_wifi_connect(void);

#endif // FAKE_ESP_WIFI_H
