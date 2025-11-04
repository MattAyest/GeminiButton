#ifndef FAKE_ESP_LOG_H
#define FAKE_ESP_LOG_H

#include <stdio.h> // Include a real header for printf

// Fake the log macros to just print to the console
#define ESP_LOGI(tag, format, ...)                                             \
  printf("[%s] (I) " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGE(tag, format, ...)                                             \
  printf("[%s] (E) " format "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, format, ...)                                             \
  printf("[%s] (W) " format "\n", tag, ##__VA_ARGS__)

#endif // FAKE_ESP_LOG_H
