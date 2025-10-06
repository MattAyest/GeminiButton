
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

// functions
//function for pool allocation and size
void* MemoryPoolFreeBlockInitiation(uint8_t BlockNumber,uint8_t BlockSize, void* FirstMemoryBlock){
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
uint8_t AllocateMemoryFromPool() {
    //should it just be given the file and i allocate it
    //should it be where it tells me the size of the file and i allocate it like that.
}


//function for pool deallocation
//function for unit test that will be moved to the other c file