#ifndef FAKE_NVS_FLASH_H
#define FAKE_NVS_FLASH_H

#include <stdint.h> // For int32_t

// --- Add fake types your code needs ---
// This should really be in a central "fake_esp_types.h"
// but we'll define it here for now.
#ifndef FAKE_ESP_ERR_T
#define FAKE_ESP_ERR_T
typedef int32_t esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#endif

// --- Fake error codes your code checks for ---
// The actual values don't matter for the compiler
#define ESP_ERR_NVS_NO_FREE_PAGES 0x1101
#define ESP_ERR_NVS_NEW_VERSION_FOUND 0x1102

// --- Add fake function prototypes ---
esp_err_t nvs_flash_init(void);
esp_err_t nvs_flash_erase(void);

#endif // FAKE_NVS_FLASH_H
