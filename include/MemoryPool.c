
#include "MemoryPool.h" 
#include <stdlib.h>


// block size declaration
#define SMALL_BLOCK_SIZE 64
#define MEDIUM_BLOCK_SIZE 512
#define LARGE_BLOCK_SIZE 2048

typedef struct{ //genertic pool info 
    void* Free_Block_Location;
    uint8_t* Pool_Start_Address;
    size_t   Total_Blocks;
} Pool_Info;

typedef struct{//nested structs for storage information
    Pool_Info Small_Pool_Storage; 
    Pool_Info Medium_Pool_Storage; 
    Pool_Info Large_Pool_Storage;
    size_t Total_Pool_Size;
    uint8_t* Memory_Claim_Address;
}  Pool_Memory_Info;

// Define a struct to overlay on each block to create the linked list
typedef struct Free_Block {
    struct Free_Block* next;//pointer pointing to the next free memory block 
} Free_Block;

//PROTOTYPES
Pool_Memory_Info* Pool_Ini(size_t num_small_blocks, size_t num_medium_blocks, size_t num_large_blocks);
void Pool_Destroy(Pool_Memory_Info* handle);
void* Pool_Alloc(size_t Memory_Size, Pool_Memory_Info* handle);
void Pool_Free(void* Packet, Pool_Memory_Info* handle);

static void* Internal_Pool_Memory_Block_Ini(size_t Block_Number,size_t Block_Size, void* First_Memory_Block);
static void* Internal_Pool_Allocation(Pool_Info* First_Memory_Block);


//FUNCTIONS
Pool_Memory_Info* Pool_Ini(size_t num_small_blocks, size_t num_medium_blocks, size_t num_large_blocks){
    Pool_Memory_Info* Memory_Handler = malloc(sizeof(Pool_Memory_Info));
    if (Memory_Handler == NULL){
        return NULL;//allocation failed
        //free main memory allocation 
    }

    Memory_Handler->Total_Pool_Size = (num_small_blocks*SMALL_BLOCK_SIZE) + (num_medium_blocks*MEDIUM_BLOCK_SIZE) + (num_large_blocks*LARGE_BLOCK_SIZE);
    Memory_Handler->Memory_Claim_Address = malloc(Memory_Handler->Total_Pool_Size);
    //check that malloc worked correctly
    if (Memory_Handler->Memory_Claim_Address == NULL){
        free(Memory_Handler);//free memory of struct on exit
        return NULL;
    }

    //filling in the struct will all relevant information
    Memory_Handler->Small_Pool_Storage.Total_Blocks = num_small_blocks;
    Memory_Handler->Medium_Pool_Storage.Total_Blocks = num_medium_blocks;
    Memory_Handler->Large_Pool_Storage.Total_Blocks = num_large_blocks;
    //initial start position of each Pool
    Memory_Handler->Small_Pool_Storage.Pool_Start_Address = Memory_Handler->Memory_Claim_Address;
    Memory_Handler->Medium_Pool_Storage.Pool_Start_Address = (Memory_Handler->Memory_Claim_Address + (num_small_blocks*SMALL_BLOCK_SIZE));
    Memory_Handler->Large_Pool_Storage.Pool_Start_Address = (Memory_Handler->Memory_Claim_Address + (num_medium_blocks*MEDIUM_BLOCK_SIZE) + (num_small_blocks*SMALL_BLOCK_SIZE));
    //Allocate the pools and return the next free block as a location 
    Memory_Handler->Small_Pool_Storage.Free_Block_Location = Internal_Pool_Memory_Block_Ini(Memory_Handler->Small_Pool_Storage.Total_Blocks, SMALL_BLOCK_SIZE, Memory_Handler->Small_Pool_Storage.Pool_Start_Address);
    Memory_Handler->Medium_Pool_Storage.Free_Block_Location = Internal_Pool_Memory_Block_Ini(Memory_Handler->Medium_Pool_Storage.Total_Blocks, MEDIUM_BLOCK_SIZE, Memory_Handler->Medium_Pool_Storage.Pool_Start_Address);
    Memory_Handler->Large_Pool_Storage.Free_Block_Location = Internal_Pool_Memory_Block_Ini(Memory_Handler->Large_Pool_Storage.Total_Blocks, LARGE_BLOCK_SIZE, Memory_Handler->Large_Pool_Storage.Pool_Start_Address);

    return Memory_Handler;
}

//function for pool allocation and size
static void* Internal_Pool_Memory_Block_Ini(size_t Block_Number,size_t Block_Size, void* First_Memory_Block){
   uint8_t* start_addr = (uint8_t*)First_Memory_Block;
   for (size_t i = 0; i < Block_Number; i++){
        uint8_t* current_addr_raw = start_addr + (i * Block_Size);
        Free_Block* current_block = (Free_Block*)current_addr_raw;

        if (i == Block_Number - 1){
            current_block->next = NULL;
        } else {
            uint8_t* next_addr_raw = start_addr + ((i + 1) * Block_Size);
            current_block->next = (Free_Block*)next_addr_raw;
        }
   }
   return First_Memory_Block;
}

void Pool_Destroy(Pool_Memory_Info* handle) {
    if (handle == NULL) return;
    free(handle->Memory_Claim_Address); // Free the main storage
    free(handle);                 // Free the handle itself
}

//function for pool allocation and size
void* Pool_Alloc(size_t Memory_Size, Pool_Memory_Info* handle) {
    //Get the size that needs to be allocated
    if (Memory_Size <= 0) {
        return NULL;
    }
    //need to update with the new pool freeblock location adress
    if (Memory_Size <= SMALL_BLOCK_SIZE) {
        return Internal_Pool_Allocation(&handle->Small_Pool_Storage);
    } else if (Memory_Size <= MEDIUM_BLOCK_SIZE) {
        return Internal_Pool_Allocation(&handle->Medium_Pool_Storage);
    } else if (Memory_Size <= LARGE_BLOCK_SIZE) {
        return Internal_Pool_Allocation(&handle->Large_Pool_Storage);
    } else {
        return NULL; // Size too large
    }
}

//Memory handler 
static void* Internal_Pool_Allocation(Pool_Info* First_Memory_Block){
    if (First_Memory_Block->Free_Block_Location == NULL) {
        return NULL; // Pool is empty, allocation fails
    }
    //get next free block from small pool storage location
    Free_Block* block_to_return  = (Free_Block*)First_Memory_Block->Free_Block_Location;
    //read location stored in next block 
    void* new_head = (void*)block_to_return->next;
    First_Memory_Block->Free_Block_Location = new_head;
    //correct next free block in linked list
    return (void*)block_to_return;
}

//function for pool deallocation
void Pool_Free(void* Packet, Pool_Memory_Info* handle){
    if (handle == NULL || Packet == NULL){
        return;
    }
    //compare the address range of the returned pointer to the start address of each pool
    //cast for all the pointer arithmatic 
    uint8_t* pointer = (uint8_t*)Packet;
    Pool_Info* SmallPoolInfo = &handle->Small_Pool_Storage;
    uint8_t* Small_Pool_Start_Pointer = handle->Small_Pool_Storage.Pool_Start_Address;
    size_t Small_Pool_Size = handle->Small_Pool_Storage.Total_Blocks * SMALL_BLOCK_SIZE; 
    //get the pointer as a value. range each value against each one
    if (pointer >= Small_Pool_Start_Pointer && pointer < (Small_Pool_Start_Pointer + Small_Pool_Size)){
        Free_Block* Return_Block_Free  = (Free_Block*)Packet;
        Return_Block_Free->next = SmallPoolInfo->Free_Block_Location;
        SmallPoolInfo->Free_Block_Location = Return_Block_Free;
        return;
    }

    Pool_Info* Medium_Pool_Info = &handle->Medium_Pool_Storage;
    size_t Medium_Pool_Size = Medium_Pool_Info->Total_Blocks * MEDIUM_BLOCK_SIZE; 
    uint8_t* Medium_Pool_Start_Pointer = handle->Medium_Pool_Storage.Pool_Start_Address;
    //go to return function providing the struct size that needs to be returned
    if (pointer >= Medium_Pool_Start_Pointer && pointer < (Medium_Pool_Start_Pointer + Medium_Pool_Size)){
        Free_Block* Return_Block_Free  = (Free_Block*)Packet;
        Return_Block_Free->next = Medium_Pool_Info->Free_Block_Location;
        Medium_Pool_Info->Free_Block_Location = Return_Block_Free;
        return;
    }
    
    Pool_Info* Large_Pool_Info = &handle->Large_Pool_Storage;
    size_t Large_Pool_Size = Large_Pool_Info->Total_Blocks * LARGE_BLOCK_SIZE; 
    uint8_t* Large_Pool_Start_Pointer = handle->Large_Pool_Storage.Pool_Start_Address;
    //go to return function providing the struct size that needs to be returned
    if (pointer >= Large_Pool_Start_Pointer && pointer < (Large_Pool_Start_Pointer + Large_Pool_Size)){
        Free_Block* Return_Block_Free  = (Free_Block*)Packet;
        Return_Block_Free->next = Large_Pool_Info->Free_Block_Location;
        Large_Pool_Info->Free_Block_Location = Return_Block_Free;
        return;
    }

    //work out the maximum pointer value and if above that throw an error?
    return;
}