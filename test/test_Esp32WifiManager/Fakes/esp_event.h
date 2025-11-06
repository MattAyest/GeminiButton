#ifndef FAKE_ESP_EVENT_H
#define FAKE_ESP_EVENT_H

#include "fake_esp_common.h"
#include "freertos/event_groups.h"

EventGroupHandle_t xEventGroupCreate(void);
EventBits_t xEventGroupWaitBits(EventGroupHandle_t, const EventBits_t,
                                const BaseType_t, const BaseType_t, TickType_t);
EventBits_t xEventGroupSetBits(EventGroupHandle_t, const EventBits_t);
EventBits_t xEventGroupClearBits(EventGroupHandle_t, const EventBits_t);

#endif
