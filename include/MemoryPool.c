
#include "MemoryPool.h" 

// block size declaration
#define SMALL_BLOCK_SIZE 64
#define MEDIUM_BLOCK_SIZE 512
#define LARGE_BLOCK_SIZE 2048

typedef struct{ //genertic pool info 
    void* FreeBlockLocation;
    uint8_t* PoolStartAdr;
    size_t   TotalBlocks;
} PoolInfo;

typedef struct{//nested structs for storage information
    PoolInfo SmallPoolStorage; 
    PoolInfo MediumPoolStorage; 
    PoolInfo LargePoolStorage;
    size_t TotalPoolSize;
    uint8_t* MemoryClaimAdr;
}  PoolMemoryInfo;

// Define a struct to overlay on each block to create the linked list
typedef struct FreeBlock {
    struct FreeBlock* next;//pointer pointing to the next free memory block 
} FreeBlock;

//FUNCTIONS

PoolMemoryInfo* PoolIni(size_t num_small_blocks, size_t num_medium_blocks, size_t num_large_blocks){
    PoolMemoryInfo* MemoryHandler = malloc(sizeof(PoolMemoryInfo));
    if (MemoryHandler == NULL){
        return NULL;//allocation failed
        //free main memory allocation 
    }

    MemoryHandler->TotalPoolSize = (num_small_blocks*SMALL_BLOCK_SIZE) + (num_medium_blocks*MEDIUM_BLOCK_SIZE) + (num_large_blocks*LARGE_BLOCK_SIZE);
    MemoryHandler->MemoryClaimAdr = malloc(MemoryHandler->TotalPoolSize);
    //check that malloc worked correctly
    if (MemoryHandler->MemoryClaimAdr == NULL){
        free(MemoryHandler);//free memory of struct on exit
        return NULL;
    }

    //filling in the struct will all relevant information
    MemoryHandler->SmallPoolStorage.TotalBlocks = num_small_blocks;
    MemoryHandler->MediumPoolStorage.TotalBlocks = num_medium_blocks;
    MemoryHandler->LargePoolStorage.TotalBlocks = num_large_blocks;
    //initial start position of each Pool
    MemoryHandler->SmallPoolStorage.PoolStartAdr = MemoryHandler->MemoryClaimAdr;
    MemoryHandler->MediumPoolStorage.PoolStartAdr = (MemoryHandler->MemoryClaimAdr + num_small_blocks*SMALL_BLOCK_SIZE);
    MemoryHandler->LargePoolStorage.PoolStartAdr = (MemoryHandler->MemoryClaimAdr + num_medium_blocks*MEDIUM_BLOCK_SIZE + num_small_blocks*SMALL_BLOCK_SIZE);
    //initial positions of each free block
    MemoryHandler->SmallPoolStorage.freeBlockLocation = MemoryHandler->SmallPoolStorage.PoolStartAdr;
    MemoryHandler->MediumPoolStorage.freeBlockLocation = MemoryHandler->MediumPoolStorage.PoolStartAdr;
    MemoryHandler->LargePoolStorage.freeBlockLocation = MemoryHandler->LargePoolStorage.PoolStartAdr;
    //Allocate the pools and return the next free block as a location 
    MemoryPoolFreeBlockInitiation(MemoryHandler->SmallPoolStorage.TotalBlocks, SMALL_BLOCK_SIZE, MemoryHandler->SmallPoolStorage.PoolStartAdr);
    MemoryPoolFreeBlockInitiation(MemoryHandler->MediumPoolStorage.TotalBlocks, MEDIUM_BLOCK_SIZE, MemoryHandler->MediumPoolStorage.PoolStartAdr);
    MemoryPoolFreeBlockInitiation(MemoryHandler->LargePoolStorage.TotalBlocks, LARGE_BLOCK_SIZE, MemoryHandler->LargePoolStorage.PoolStartAdr);

    return MemoryHandler;
}

//function for pool allocation and size
void* MemoryPoolFreeBlockInitiation(size_t BlockNumber,size_t BlockSize, void* FirstMemoryBlock){
   uint8_t* start_addr = (uint8_t*)FirstMemoryBlock;
   for (size_t i = 0; i < BlockNumber; i++){
        uint8_t* current_addr_raw = start_addr + (i * BlockSize);
        FreeBlock* current_block = (FreeBlock*)current_addr_raw;

        if (i == BlockNumber - 1){
            current_block->next = NULL;
        } else {
            uint8_t* next_addr_raw = start_addr + ((i + 1) * BlockSize);
            current_block->next = (FreeBlock*)next_addr_raw;
        }
   }
   return FirstMemoryBlock;
}

void PoolDestroy(void* FirstMemoryBlock, void* Memoryhandle){
    free(FirstMemoryBlock);
    free(Memoryhandle);
}





//function for pool allocation and size
void* AllocateMemoryFromPool(size_t MemorySize, PoolMemoryInfo* handle) {
    //Get the size that needs to be allocated
    if (MemorySize <= 0) {
        return NULL;
    }
    //need to update with the new pool freeblock location adress
    if (MemorySize <= SMALL_BLOCK_SIZE) {
        return  PoolAllocation(&handle->SmallPoolStorage);
    }
    if (MemorySize <= MEDIUM_BLOCK_SIZE) {
            return PoolAllocation(&handle->MediumPoolStorage);
    }
    if (MemorySize <= LARGE_BLOCK_SIZE) {
            return PoolAllocation(&handle->LargePoolStorage);
    } else {
        return NULL;
    }
}

//Memory handler 
uint8_t* PoolAllocation(PoolInfo* FirstMemoryBlock){
    //get next free block from small pool storage location
    FreeBlock* block_to_return  = (FreeBlock*)FirstMemoryBlock->FreeBlockLocation;
    //read location stored in next block 
    void* new_head = (void*)block_to_return->next;
    FirstMemoryBlock->FreeBlockLocation = new_head;
    //correct next free block in linked list
    return (void*)block_to_return;
}

//function for pool deallocation
void FreeMemoryFromPool(void* Packet, PoolMemoryInfo* handle){
    if (handle == NULL || Packet == NULL){
        return;
    }
    //compare the address range of the returned pointer to the start address of each pool
    //cast for all the pointer arithmatic 
    uint8_t* pointer = (uint8_t*)Packet;
    PoolInfo* SmallPoolInfo = &handle->SmallPoolStorage;
    uint8_t* SmallPoolStartPointer = handle->SmallPoolStorage.PoolStartAdr;
    size_t SmallPoolSize = handle->SmallPoolStorage.TotalBlocks * SMALL_BLOCK_SIZE; 
    //get the pointer as a value. range each value against each one
    if (pointer >= SmallPoolStartPointer && pointer < (SmallPoolStartPointer + SmallPoolSize)){
        FreeBlock* ReturnBlockFree  = (FreeBlock*)Packet;
        ReturnBlockFree->next = SmallPoolInfo->FreeBlockLocation;
        SmallPoolInfo->FreeBlockLocation = ReturnBlockFree;
        return;
    }

    PoolInfo* MediumPoolInfo = &handle->MediumPoolStorage;
    size_t MediumPoolSize = MediumPoolInfo->TotalBlocks * MEDIUM_BLOCK_SIZE; 
    uint8_t* MediumPoolStartPointer = handle->PoolStartAdr + SmallPoolSize + MediumPoolSize;
    //go to return function providing the struct size that needs to be returned
    if (pointer >= MediumPoolStartPointer && pointer < (MediumPoolStartPointer + MediumPoolSize)){
        FreeBlock* ReturnBlockFree  = (FreeBlock*)Packet;
        ReturnBlockFree->next = MediumPoolInfo->FreeBlockLocation;
        MediumPoolInfo->FreeBlockLocation = ReturnBlockFree;
        return;
    }
    
    PoolInfo* LargePoolInfo = &handle->LargePoolStorage;
    size_t LargePoolSize = LargePoolInfo->TotalBlocks * LARGE_BLOCK_SIZE; 
    uint8_t* LargePoolStartPointer = handle->PoolStartAdr + SmallPoolSize + MediumPoolSize + LargePoolSize;
    //go to return function providing the struct size that needs to be returned
    if (pointer >= LargePoolStartPointer && pointer < (LargePoolStartPointer + LargePoolSize)){
        FreeBlock* ReturnBlockFree  = (FreeBlock*)Packet;
        ReturnBlockFree->next = LargePoolInfo->FreeBlockLocation;
        LargePoolInfo->FreeBlockLocation = ReturnBlockFree;
        return;
    }

    //work out the maximum pointer value and if above that throw an error?
    return;
}