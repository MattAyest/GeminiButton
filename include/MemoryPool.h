// MemoryPool.h

#ifndef MEMORY_POOL_H  // A unique include guard
#define MEMORY_POOL_H

#include <stdint.h>
#include <stddef.h>
#include <MemoryPool.c>


// The PUBLIC functions that users can call
PoolMemoryInfo* PoolIni(size_t num_small_blocks, size_t num_medium_blocks, size_t num_large_blocks);
void* PoolAlloc(size_t MemorySize, PoolMemoryInfo* handle);
void PoolFree(void* Packet, PoolMemoryInfo* handle);
void PoolDestroy(PoolMemoryInfo* handle) ;

#endif // MEMORY_POOL_H