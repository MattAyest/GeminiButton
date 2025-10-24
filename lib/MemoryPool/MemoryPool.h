// MemoryPool.h

#ifndef MEMORY_POOL_H // A unique include guard
#define MEMORY_POOL_H

#include <stddef.h>
#include <stdint.h>

typedef struct { // genertic pool info
  void *Free_Block_Location;
  uint8_t *Pool_Start_Address;
  size_t Total_Blocks;
} PoolInfo;

typedef struct { // nested structs for storage information
  PoolInfo Small_Pool_Storage;
  PoolInfo Medium_Pool_Storage;
  PoolInfo Large_Pool_Storage;
  size_t Total_Pool_Size;
  uint8_t *Memory_Claim_Address;
} PoolMemoryInfo;

// Define a struct to overlay on each block to create the linked list
typedef struct FreeBlock {
  struct FreeBlock *next; // pointer pointing to the next free memory block
} FreeBlock;

// The PUBLIC functions that users can call
PoolMemoryInfo *Pool_Ini(size_t num_small_blocks, size_t num_medium_blocks,
                         size_t num_large_blocks);
void *Pool_Alloc(size_t MemorySize, PoolMemoryInfo *handle);
void Pool_Free(void *Packet, PoolMemoryInfo *handle);
void Pool_Destroy(PoolMemoryInfo *handle);

#endif // MEMORY_POOL
