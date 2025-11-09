/*
 *description: code for faking the esp_event.c library for testing
 *creator:Matthew Ayestaran
 *Date:10/25/25
 * */

#ifndef FAKE_ESP_EVENT_H
#define FAKE_ESP_EVENT_H

#include "fake_esp_common.h"
#include "fff.h"

#define IP_EVENT "IP_EVENT"
#define IP_EVENT_STA_GOT_IP 1

typedef struct {
  struct {
    struct {
      uint32_t addr;
    } ip;
  } ip_info;
} ip_event_got_ip_t;

#define IPSTR "%d.%d.%d.%d"
#define IP2STR(ip) 0, 0, 0, 0

// This is the one we will implement as a custom "spy"
esp_err_t esp_event_handler_register(esp_event_base_t, int32_t,
                                     esp_event_handler_t, void *);
// This one can be a normal fake
DECLARE_FAKE_VALUE_FUNC(esp_err_t, esp_event_loop_create_default);

#endif
