/*MemoryPool Control for esp32 
    Written by Matthew Ayestaran
    Date 04/10/2025
    purpose: this contains the unit tests for the memory control header
*/

//Filling blocks with known repeating 01 that can be written as 0xAA

//#ifdef UNIT_TEST
#include <unity.h>
#include <MemoryPool.h>

static PoolMemoryInfo* MemoryHandler;
const uint8_t TEST_PATTERN = 0xAA;
// block size declaration
#define SMALL_BLOCK_SIZE 64
#define MEDIUM_BLOCK_SIZE 512
#define LARGE_BLOCK_SIZE 2048


void setUp(void) {
    MemoryHandler = PoolIni(5,5,5);//small number of blocks so works on all systems
}

void tearDown(void) {
    pool_destroy(MemoryHandler);
}

void helper_test_Memorypool_Gives_Pointer_that_Can_Be_Used(size_t MemorySize, size_t BlockSize){
    //Get memory allocations
    void* BlockAdressPointer = AllocateMemoryFromPool(MemorySize,MemoryHandler);
    TEST_ASSERT_NOT_NULL(BlockAdressPointer);

    //setting chunks to read in as 1 byte
    uint8_t* MemoryBlockChunk = (uint8_t*)BlockAdressPointer;

    //setting each byte to 0xAA then confirming
    memset(MemoryBlockChunk, TEST_PATTERN, BlockSize);
    for (size_t i = 0; i < BlockSize; i++) {
        TEST_ASSERT_EQUAL_UINT8(TEST_PATTERN, MemoryBlockChunk[i]);
    }
    //free up block
    FreeMemoryFromPool(BlockAdressPointer, MemoryHandler);
}

void Small_Testing_Overspill_To_next_Block(){

}

void Small_confirm_memory_Returns_Same_Pointer_After_deallocation(void) {
    // ask to allocate memorypool and confirm it is working;
    u_int8_t* SmallBlockAdress = AllocateMemoryFromPool(25,MemoryHandler);
    //Return memory block 
    //get new block adress
    //test it is confirmed
}


void test_Small_Block_Test_Group(){
    helper_test_Memorypool_Gives_Pointer_that_Can_Be_Used((SMALL_BLOCK_SIZE-1), SMALL_BLOCK_SIZE);
    helper_test_Memorypool_Gives_Pointer_that_Can_Be_Used((SMALL_BLOCK_SIZE), SMALL_BLOCK_SIZE);
}
void test_Medium_Block_Test_Group(){
    helper_test_Memorypool_Gives_Pointer_that_Can_Be_Used((MEDIUM_BLOCK_SIZE-1), MEDIUM_BLOCK_SIZE);
    helper_test_Memorypool_Gives_Pointer_that_Can_Be_Used((MEDIUM_BLOCK_SIZE), MEDIUM_BLOCK_SIZE);
}
void test_Large_Block_Test_Group(){
    helper_test_Memorypool_Gives_Pointer_that_Can_Be_Used((LARGE_BLOCK_SIZE-1), LARGE_BLOCK_SIZE);
    helper_test_Memorypool_Gives_Pointer_that_Can_Be_Used((LARGE_BLOCK_SIZE), LARGE_BLOCK_SIZE);
}
void app_main(void) { // On ESP32, or use main() for native tests
    UNITY_BEGIN(); // Starts the test runner

    // Run tests that memory gets allocated correctly
    RUN_TEST(test_Small_Block_Test_Group);
    RUN_TEST(test_Medium_Block_Test_Group);
    RUN_TEST(test_Large_Block_Test_Group);

    UNITY_END(); // Ends the test runner and prints a summary
}
//#endif