#include <stdint.h> // Provides uint32_t, etc.

/* * --- FAKE FREETOS TYPES ---
 * These are minimal definitions just for native testing.
 */
typedef void *EventGroupHandle_t;
typedef uint32_t EventBits_t;
typedef int BaseType_t;
typedef uint32_t TickType_t;

// Define constants that might be used
#define pdTRUE 1
#define pdFALSE 0

/* --- END OF FAKE TYPES --- */

// --- Your Fake Function Implementations ---

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

// ... etc. for your other fake functions
