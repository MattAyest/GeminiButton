// MemoryPool.h

#ifndef MEMORY_POOL_H  // A unique include guard
#define MEMORY_POOL_H

#include <stdint.h>
#include <stddef.h>
#include "MemoryPool.c"


// The PUBLIC functions that users can call
PoolMemoryInfo* Pool_Ini(size_t num_small_blocks, size_t num_medium_blocks, size_t num_large_blocks);
void* Pool_Alloc(size_t MemorySize, PoolMemoryInfo* handle);
void Pool_Free(void* Packet, PoolMemoryInfo* handle);
void Pool_Destroy(PoolMemoryInfo* handle) ;

#endif // MEMORY_POOL