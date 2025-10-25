/*
 *Description: code to fake the nvs_flash.c library for testing
 *Creator:Matthew Ayestaran
 *Date: 10/25/25
 * */

// test/test_wifi_manager/fakes/fake_nvs_flash.c
#include "nvs_flash.h" // Include real header for types/return codes

// --- Global variables to control or check behavior ---
static esp_err_t g_nvs_init_result =
    ESP_OK; // Let tests control the return value
int g_nvs_init_call_count = 0;
int g_nvs_erase_call_count = 0;

// --- Test helper functions ---
void fake_nvs_flash_init_set_result(esp_err_t result) {
  g_nvs_init_result = result;
}
void fake_nvs_flash_reset_counts(void) {
  g_nvs_init_call_count = 0;
  g_nvs_erase_call_count = 0;
}

// --- Fake implementations ---
esp_err_t nvs_flash_init(void) {
  g_nvs_init_call_count++;
  return g_nvs_init_result; // Return the value set by the test
}

esp_err_t nvs_flash_erase(void) {
  g_nvs_erase_call_count++;
  return ESP_OK; // Assume erase always works
}
