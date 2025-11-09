#include "esp_netif.h"
#include "fake_esp_common.h"

FAKE_VALUE_FUNC(esp_err_t, esp_netif_init);
FAKE_VALUE_FUNC(esp_netif_t *, esp_netif_create_default_wifi_sta);
