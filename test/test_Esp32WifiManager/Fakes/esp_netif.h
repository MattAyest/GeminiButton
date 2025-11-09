#ifndef FAKE_ESP_NETIF_H
#define FAKE_ESP_NETIF_H

#include "fake_esp_common.h"
#include "fff.h"
#include <stdint.h> // For int32_t

DECLARE_FAKE_VALUE_FUNC(esp_err_t, esp_netif_init)
DECLARE_FAKE_VALUE_FUNC(esp_netif_t *, esp_netif_create_default_wifi_sta)

#endif // FAKE_ESP_NETIF_H
