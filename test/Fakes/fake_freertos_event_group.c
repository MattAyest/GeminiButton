/*
 * description: code to fake the freertos_event_groups.c library for testing
 * creator: Matthew Ayestaran
 * Date:10/25/2025
 * */
#include "freertos/freeRTOS.h"

#include "freertos/event_groups.h" // Include real header for types

// --- Fake Data (Optional, for advanced tests later) ---
// static EventGroupHandle_t dummy_event_group = (EventGroupHandle_t)1; // Fake
// handle static EventBits_t current_bits = 0;

// --- Minimal Fake Implementations ---

EventGroupHandle_t xEventGroupCreate(void) {
  // Return a non-NULL dummy handle. Cast 1 to the handle type.
  return (EventGroupHandle_t)1;
}

EventBits_t xEventGroupWaitBits(EventGroupHandle_t xEventGroup,
                                const EventBits_t uxBitsToWaitFor,
                                const BaseType_t xClearOnExit,
                                const BaseType_t xWaitForAllBits,
                                TickType_t xTicksToWait) {
  // For now, just return 0. Tests can be made smarter later.
  return 0;
}

EventBits_t xEventGroupSetBits(EventGroupHandle_t xEventGroup,
                               const EventBits_t uxBitsToSet) {
  // Pretend we set the bits, return the "new" value (or 0)
  return uxBitsToSet;
}

EventBits_t xEventGroupClearBits(EventGroupHandle_t xEventGroup,
                                 const EventBits_t uxBitsToClear) {
  // Pretend we cleared the bits, return the "new" value (0)
  return 0;
}

// Add dummy implementations for any other FreeRTOS functions your code might
// call void vEventGroupDelete( EventGroupHandle_t xEventGroup ) { /* Do nothing
// */ }
