/*
 * description: header file for macros for testing the esp_log library
 * creator: Matthew Ayestaran
 * Date: 10/25/25
 *
 * */
// test/test_wifi_manager/fakes/fake_esp_log.h
#pragma once

// Make log macros do nothing in tests
#define ESP_LOGI(tag, format, ...)
#define ESP_LOGW(tag, format, ...)
#define ESP_LOGE(tag, format, ...)

// Define IPSTR/IP2STR so the compiler doesn't complain
#define IPSTR "%d.%d.%d.%d"
#define IP2STR(ip) 0, 0, 0, 0 // Dummy implementation for tests
