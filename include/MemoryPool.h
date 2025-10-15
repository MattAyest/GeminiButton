// MemoryPool.h

#ifndef MEMORY_POOL_H  // A unique include guard
#define MEMORY_POOL_H

#include <stdint.h>
#include <stddef.h>

// Opaque pointer to hide the implementation details
typedef struct PoolMemoryInfo PoolMemoryInfo;

// The PUBLIC functions that users can call
PoolMemoryInfo* pool_init(size_t num_small, size_t num_medium, size_t num_large);
void* pool_alloc(PoolMemoryInfo* handle, size_t size);
void pool_free(PoolMemoryInfo* handle, void* ptr);
void pool_destroy(PoolMemoryInfo* handle);

#endif // MEMORY_POOL_H