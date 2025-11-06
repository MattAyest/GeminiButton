/*
 *Description: code to fake the nvs_flash.c library for testing
 *Creator:Matthew Ayestaran
 *Date: 10/25/25
 * */

// test/test_wifi_manager/fakes/fake_nvs_flash.c
// #include "nvs_flash.h" // Include real header for types/return codes

#include "fff.h"
#include <nvs_flash.h>

typedef int esp_rtt_t;

FAKE_VALUE_FUNC(esp_err_t, nvs_flash_init);
FAKE_VALUE_FUNC(esp_err_t, nvs_flash_erase);
