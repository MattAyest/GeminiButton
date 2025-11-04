#ifndef FAKE_EVENT_GROUPS_H
#define FAKE_EVENT_GROUPS_H

#include "freertos/FreeRTOS.h" // Include your other fake header

// Fake the types your code needs
typedef void *EventGroupHandle_t;
typedef TickType_t EventBits_t;

// Fake the #defines your code uses
#define BIT0 (1 << 0)
#define BIT1 (1 << 1)
// ...add any other BITs your code uses...

// Fake the function prototypes your code calls
EventGroupHandle_t xEventGroupCreate(void);
EventBits_t xEventGroupWaitBits(EventGroupHandle_t, const EventBits_t,
                                const BaseType_t, const BaseType_t, TickType_t);
EventBits_t xEventGroupSetBits(EventGroupHandle_t, const EventBits_t);
EventBits_t xEventGroupClearBits(EventGroupHandle_t, const EventBits_t);

#endif
