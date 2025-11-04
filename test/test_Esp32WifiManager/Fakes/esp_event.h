#ifndef FAKE_ESP_EVENT_H
#define FAKE_ESP_EVENT_H

#include <stdint.h>

// Minimal fake types
typedef const char *esp_event_base_t;
typedef void *esp_event_loop_handle_t;
typedef void (*esp_event_handler_t)(void *, esp_event_base_t, int32_t, void *);

// --- Definitions for IP Events ---
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

// Fake function prototypes
esp_err_t esp_event_handler_register(esp_event_base_t, int32_t,
                                     esp_event_handler_t, void *);
esp_err_t esp_event_loop_create_default(void);

#endif
