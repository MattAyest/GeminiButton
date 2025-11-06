/*
 *Description: code to fake the nvs_flash.c library for testing
 *Creator:Matthew Ayestaran
 *Date: 10/25/25
 * */

#ifndef FAKE_NVS_FLASH_H
#define FAKE_NVS_FLASH_H

#include "fake_esp_common.h"
#include "nvs_flash.h"

// functions called as fakes due to fake_esp_common
esp_err_t nvs_flash_init(void);
esp_err_t nvs_flash_erase(void);

#endif
