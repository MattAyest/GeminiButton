/*MemoryPool Control for esp32 
    Written by Matthew Ayestaran
    Date 04/10/2025
    purpose: this contains the unit tests for the memory control header
*/


//remaining tests

/*
    Pool Exhaustion
    Freeing NULL Pointer
    Double Free
    Interleaved Allocation/Free
    Invalid Pointer Free
*/



//#ifdef UNIT_TEST

//Filling blocks with known repeating 01 that can be written as 0xAA
#include <string.h>
#include <stdlib.h>


#include <unity.h>
#include "MemoryPool.h"

//standard values
static PoolMemoryInfo* MemoryHandler;
const uint8_t TEST_PATTERN = 0xAA;
const size_t SmallPoolSize = 5;
const size_t MediumPoolSize = 5;
const size_t LargePoolSize = 5;



//THESE NEED TO MATCH THE .C FILE DEFINITIONS
#define SMALL_BLOCK_SIZE 64
#define MEDIUM_BLOCK_SIZE 512
#define LARGE_BLOCK_SIZE 2048


void setUp(void) {
    //TODO: check this returns null
    MemoryHandler = PoolIni(SmallPoolSize,MediumPoolSize,LargePoolSize);//small number of blocks so works on all systems
    TEST_ASSERT_NOT_NULL(MemoryHandler);
}

void tearDown(void) {
    PoolDestroy(MemoryHandler);
}

void test_Small_Pool_Exhaustion(){
    //see what happens if you pull more than the max pool ini values
    //loop pool allocation until poolsize +1
    void* BlockAdressPointer [SmallPoolSize];
    for (size_t i = 0; i < SmallPoolSize; i++) {
        BlockAdressPointer[i] = PoolAlloc(SMALL_BLOCK_SIZE,MemoryHandler);
        TEST_ASSERT_NOT_NULL_MESSAGE(BlockAdressPointer[i], "Allocation failed unexpectedly before pool was full.");
    }
    
    TEST_ASSERT_NULL_MESSAGE(PoolAlloc(SMALL_BLOCK_SIZE,MemoryHandler), "Allocation succeeded unexpectedly when pool should be full.");

    for (int i = 0; i < (SmallPoolSize); ++i) {
        PoolFree(BlockAdressPointer[i], MemoryHandler);
    }

    
}


void helper_Confirm_valid_Pool_Creation(size_t MemorySize, size_t BlockSize){
    //Get memory allocations
    void* BlockAdressPointer = PoolAlloc(MemorySize,MemoryHandler);
    TEST_ASSERT_NOT_NULL_MESSAGE(BlockAdressPointer,  "Allocation failed unexpectedly before pool was full.");

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
    helper_Confirm_valid_Pool_Creation((SMALL_BLOCK_SIZE-1), SMALL_BLOCK_SIZE);
    helper_Confirm_valid_Pool_Creation((SMALL_BLOCK_SIZE), SMALL_BLOCK_SIZE);
}
void test_Medium_Block_Group(){
    helper_Confirm_valid_Pool_Creation((MEDIUM_BLOCK_SIZE-1), MEDIUM_BLOCK_SIZE);
    helper_Confirm_valid_Pool_Creation((MEDIUM_BLOCK_SIZE), MEDIUM_BLOCK_SIZE);
}
void test_Large_Block_Group(){
    helper_Confirm_valid_Pool_Creation((LARGE_BLOCK_SIZE-1), LARGE_BLOCK_SIZE);
    helper_Confirm_valid_Pool_Creation((LARGE_BLOCK_SIZE), LARGE_BLOCK_SIZE);
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




int main(void) {

    UNITY_BEGIN(); // Starts the test runner

    // Run tests that memory gets allocated correctly
    RUN_TEST(test_Small_Block_Group);
    RUN_TEST(test_Medium_Block_Group);
    RUN_TEST(test_Large_Block_Group);
    RUN_TEST(test_Small_reallocation);
    RUN_TEST(test_Medium_reallocation);
    RUN_TEST(test_Large_reallocation);
    RUN_TEST(test_Small_Pool_Exhaustion);
    return UNITY_END(); // Ends the test runner and prints a summary
}
//#endif