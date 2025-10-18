/*MemoryPool Control for esp32 
    Written by Matthew Ayestaran
    Date 04/10/2025
    purpose: this contains the unit tests for the memory control header
*/
//#ifdef UNIT_TEST

//Filling blocks with known repeating 01 that can be written as 0xAA
#include <string.h>
#include <stdlib.h>


#include <unity.h>
#include "MemoryPool.h"

static PoolMemoryInfo* MemoryHandler;
const uint8_t TEST_PATTERN = 0xAA;
// block size declaration

//THESE NEED TO MATCH THE .C FILE DEFINITIONS
#define SMALL_BLOCK_SIZE 64
#define MEDIUM_BLOCK_SIZE 512
#define LARGE_BLOCK_SIZE 2048


void setUp(void) {
    //TODO: check this returns null
    MemoryHandler = PoolIni(5,5,5);//small number of blocks so works on all systems
    TEST_ASSERT_NOT_NULL(MemoryHandler);
}

void tearDown(void) {
    PoolDestroy(MemoryHandler);
}

void helper_test_Memorypool_Gives_Pointer_that_Can_Be_Used(size_t MemorySize, size_t BlockSize){
    //Get memory allocations
    void* BlockAdressPointer = PoolAlloc(MemorySize,MemoryHandler);
    TEST_ASSERT_NOT_NULL(BlockAdressPointer);

    //setting chunks to read in as 1 byte
    uint8_t* MemoryBlockChunk = (uint8_t*)BlockAdressPointer;

    //setting each byte to 0xAA then confirming
    memset(MemoryBlockChunk, TEST_PATTERN, BlockSize);
    for (size_t i = 0; i < BlockSize; i++) {
        TEST_ASSERT_EQUAL_UINT8(TEST_PATTERN, MemoryBlockChunk[i]);
    }
    //free up block
    PoolFree(BlockAdressPointer, MemoryHandler);
}

//test to see if it returns the same value  

void helper_freeblock_system(size_t MemorySize){
    //get block and remember adress location
    void* BlockAdressPointer = PoolAlloc(MemorySize,MemoryHandler);
    TEST_ASSERT_NOT_NULL(BlockAdressPointer);
    void* MemoryAddressTempStorage = BlockAdressPointer;
    PoolFree(BlockAdressPointer, MemoryHandler);

    //try again and compare values
    BlockAdressPointer = PoolAlloc(MemorySize,MemoryHandler);
    TEST_ASSERT_NOT_NULL(BlockAdressPointer);
    TEST_ASSERT_EQUAL_PTR(MemoryAddressTempStorage, BlockAdressPointer);
    PoolFree(BlockAdressPointer, MemoryHandler);
}

void test_Small_Block_Group(){
    helper_Memorypool_Gives_Pointer_that_Can_Be_Used((SMALL_BLOCK_SIZE-1), SMALL_BLOCK_SIZE);
    helper_Memorypool_Gives_Pointer_that_Can_Be_Used((SMALL_BLOCK_SIZE), SMALL_BLOCK_SIZE);
}
void test_Medium_Block_Group(){
    helper_Memorypool_Gives_Pointer_that_Can_Be_Used((MEDIUM_BLOCK_SIZE-1), MEDIUM_BLOCK_SIZE);
    helper_Memorypool_Gives_Pointer_that_Can_Be_Used((MEDIUM_BLOCK_SIZE), MEDIUM_BLOCK_SIZE);
}
void test_Large_Block_Group(){
    helper_Memorypool_Gives_Pointer_that_Can_Be_Used((LARGE_BLOCK_SIZE-1), LARGE_BLOCK_SIZE);
    helper_Memorypool_Gives_Pointer_that_Can_Be_Used((LARGE_BLOCK_SIZE), LARGE_BLOCK_SIZE);
}

void test_Small_reallocation(){
    helper_freeblock_system(SMALL_BLOCK_SIZE);
}
void test_Medium_reallocation(){
    helper_freeblock_system(MEDIUM_BLOCK_SIZE);
}
void test_Large_reallocation(){
    helper_freeblock_system(LARGE_BLOCK_SIZE);
}

int main(void) { // On ESP32, or use main() for native tests

    UNITY_BEGIN(); // Starts the test runner

    // Run tests that memory gets allocated correctly
    RUN_TEST(test_Small_Block_Group);
    RUN_TEST(test_Medium_Block_Group);
    RUN_TEST(test_Large_Block_Group);
    RUN_TEST(test_Small_reallocation);
    RUN_TEST(test_Medium_reallocation);
    RUN_TEST(test_Large_reallocation);

    return UNITY_END(); // Ends the test runner and prints a summary
}
//#endif