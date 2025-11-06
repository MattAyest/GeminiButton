/*
 * description: number of tests for the esp32wifimanager header file to confirm
 * functionality created by: matthew ayestaran date: 19-10-2025
 * */
/*
#ifndef Test_Esp32WifiManager_c

#include <unity.h>

FAKE_VALUE_FUNC(esp_err_t, esp_wifi_connect);
FAKE_VALUE_FUNC(EventBits_t, xEventGroupSetBits, EventGroupHandle_t,
                const EventBits_t);
FAKE_VALUE_FUNC(EventBits_t, xEventGroupClearBits, EventGroupHandle_t,
                const EventBits_t);

#include "esp32wifimanager.h"

#endif // fndef Test_Esp32WifiManager.c"
*/

#include "Esp32WifiManager.h"
#include "fff.h"
#include "nvs_flash.h"
#include <unity.h>

// fake functions
FAKE_VALUE_FUNC(esp_err_t, nvs_flash_init())

void setUp(void) {
  /* Reset fakes */
  // RESET_FAKE(function name)
  nvs_flash_init_fake.return_val = ESP_OK;
}
void tearDown(void) {}

void test_YourFirstWifiTest(void) {
  // ... Arrange, Act, Assert ...
  TEST_ASSERT_TRUE(1); // Placeholder test
}

// testing the wifi_manager_init_station
void test_wifi_manager_init_station() {}

int app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_YourFirstWifiTest);
  // Add RUN_TEST for all your wifi tests
  return UNITY_END();
}
