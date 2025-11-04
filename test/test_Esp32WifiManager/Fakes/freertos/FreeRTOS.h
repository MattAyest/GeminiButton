#ifndef FAKE_FREERTOS_H
#define FAKE_FREERTOS_H

#include <stdint.h> // For uint32_t

// Fake the types your code needs
typedef uint32_t TickType_t;
typedef void *TaskHandle_t;
typedef uint8_t BaseType_t;

// Fake the #defines
#define pdFALSE ((BaseType_t)0)
#define pdTRUE ((BaseType_t)1)
#define portMAX_DELAY ((TickType_t)0xFFFFFFFFUL)

#endif
