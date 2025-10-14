#ifndef MemoryPool.h
#define MemoryPool.h

#include <stdint.h>
#include <stdio.h>

struct PoolMemoryInfo;
typedef PoolMemoryInfo PoolMemoryInfo;

//functions
PoolMemoryInfo* PoolIni(size_t num_small_blocks, size_t num_medium_blocks, size_t num_large_blocks);
void* AllocateMemoryFromPool(size_t MemorySize, PoolMemoryInfo* handle);
void PoolDestroy(void* FirstMemoryBlock);

#endif