#ifndef FAKE_ESP_EVENT_H
#define FAKE_ESP_EVENT_H

#include <stdint.h> // For int32_t

// Add a fake for esp_err_t (your library probably needs this)
typedef int32_t esp_err_t;

// Minimal fake types to satisfy the compiler
typedef const char *esp_event_base_t;
typedef void *esp_event_loop_handle_t;
typedef void (*esp_event_handler_t)(void *, esp_event_base_t, int32_t, void *);

// Fake function prototypes for functions your library calls
esp_err_t esp_event_handler_register(esp_event_base_t, int32_t,
                                     esp_event_handler_t, void *);
esp_err_t esp_event_loop_create_default(void);
// ... add any other esp_event functions your code calls ...

#endif
