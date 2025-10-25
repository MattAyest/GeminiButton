/*
 * description: header file for macros for faking the esp_err library
 * creator:Matthew Ayestaran
 * Date: 10/25/25
 *
 * */

// test/test_wifi_manager/fakes/fake_esp_err.h
#pragma once
#include "esp_err.h" // Include real header for esp_err_t type

// Make ESP_ERROR_CHECK do nothing in tests
#define ESP_ERROR_CHECK(x) (void)(x)
