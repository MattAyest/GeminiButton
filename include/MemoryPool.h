// MemoryPool.h

#ifndef MEMORY_POOL_H  // A unique include guard
#define MEMORY_POOL_H

#include <stdint.h>
#include <stddef.h>

// Opaque pointer to hide the implementation details
typedef struct PoolMemoryInfo PoolMemoryInfo;

// The PUBLIC functions that users can call
PoolMemoryInfo* PoolIni(size_t num_small_blocks, size_t num_medium_blocks, size_t num_large_blocks);
void* AllocateMemoryFromPool(size_t MemorySize, PoolMemoryInfo* handle);
void FreeMemoryFromPool(void* Packet, PoolMemoryInfo* handle);
void pool_destroy(PoolMemoryInfo* handle) ;

#endif // MEMORY_POOL_H