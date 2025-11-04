#ifndef FAKE_ESP_NETIF_H
#define FAKE_ESP_NETIF_H

#include <stdint.h> // For int32_t

// --- Add fake types your code needs ---
typedef int32_t esp_err_t; // You might have this in another fake header
typedef void *esp_netif_t;

// --- Add fake function prototypes ---
esp_err_t esp_netif_init(void);
esp_netif_t *esp_netif_create_default_wifi_sta(void);

#endif // FAKE_ESP_NETIF_H
