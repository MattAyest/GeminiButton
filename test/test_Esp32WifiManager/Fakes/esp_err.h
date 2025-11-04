#ifndef FAKE_ESP_ERR_H
#define FAKE_ESP_ERR_H

#include <assert.h>
#include <stdio.h> // For printf

// Fake macro to just check the error code
#define ESP_ERROR_CHECK(x) assert(x == 0)

#endif
