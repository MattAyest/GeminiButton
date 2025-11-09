/*MemoryPool Control header for embedded systems
    Written by Matthew Ayestaran
    Date 04/10/2025
    purpose: this contains the unit tests for the memory control header
*/

#ifdef UNIT_TEST

#include "MemoryPool.h"
#include <stdlib.h>
#include <string.h>
#include <unity.h>

// THESE NEED TO MATCH THE .C FILE DEFINITIONS
#define SMALL_BLOCK_SIZE 64
#define MEDIUM_BLOCK_SIZE 512
#define LARGE_BLOCK_SIZE 2048

// standard values
static PoolMemoryInfo *Memory_Handler;
const uint8_t TEST_PATTERN =
    0xAA; // Filling blocks with known repeating 01 that can be written as 0xAA
const size_t Small_Pool_Size = 5;
const size_t Medium_Pool_Size = 5;
const size_t Large_Pool_Size = 5;

// PROTOTYPING HELPERS
void helper_Small_Pool_Exhaustion(size_t Block_Size, size_t Pool_Size);
void helper_Confirm_valid_Pool_Creation(size_t Memory_Size, size_t Block_Size);
void helper_freeblock_system(size_t Memory_Size);
void helper_double_free(size_t Memory_Size);
void helper_Interleaved_Allocation_And_Free(size_t Memory_Size);

// PROTOTYPING TESTS
void test_Small_Block_Group_Allocation();
void test_Medium_Block_Group_Allocation();
void test_Large_Block_Group_Allocation();

void test_Small_reallocation();
void test_Medium_reallocation();
void test_Large_reallocation();

void test_small_exhaution();
void test_medium_exhaution();
void test_large_exhaution();

void test_large_double_free();
void test_medium_double_free();
void test_small_double_free();

void test_Interleaved_Small_Allocation_And_Free();
void test_Interleaved_Medium_Allocation_And_Free();
void test_Interleaved_Large_Allocation_And_Free();

void test_free_invalid_pointer_does_not_corrupt_list();

void test_Freeing_Null_Pointer();

//================================CODE
// START=============================================
void setUp(void) {
  // TODO: check this returns null
  Memory_Handler = Pool_Ini(
      Small_Pool_Size, Medium_Pool_Size,
      Large_Pool_Size); // small number of blocks so works on all systems
  TEST_ASSERT_NOT_NULL(Memory_Handler);
}
void tearDown(void) { Pool_Destroy(Memory_Handler); }

int main(void) {

  UNITY_BEGIN(); // Starts the test runner

  // Run tests that memory gets allocated correctly
  RUN_TEST(test_Small_Block_Group_Allocation);
  RUN_TEST(test_Medium_Block_Group_Allocation);
  RUN_TEST(test_Large_Block_Group_Allocation);

  RUN_TEST(test_Small_reallocation);
  RUN_TEST(test_Medium_reallocation);
  RUN_TEST(test_Large_reallocation);

  RUN_TEST(test_small_exhaution);
  RUN_TEST(test_medium_exhaution);
  RUN_TEST(test_large_exhaution);

  RUN_TEST(test_small_double_free);
  RUN_TEST(test_medium_double_free);
  RUN_TEST(test_large_double_free);

  RUN_TEST(test_Interleaved_Small_Allocation_And_Free);
  RUN_TEST(test_Interleaved_Medium_Allocation_And_Free);
  RUN_TEST(test_Interleaved_Large_Allocation_And_Free);

  RUN_TEST(test_free_invalid_pointer_does_not_corrupt_list);

  RUN_TEST(test_Freeing_Null_Pointer);

  return UNITY_END(); // Ends the test runner and prints a summary
}

// TEST FUNCTIONS
// Allocation Test Group
void test_Small_Block_Group_Allocation() {
  helper_Confirm_valid_Pool_Creation((SMALL_BLOCK_SIZE - 1), SMALL_BLOCK_SIZE);
  helper_Confirm_valid_Pool_Creation((SMALL_BLOCK_SIZE), SMALL_BLOCK_SIZE);
}
void test_Medium_Block_Group_Allocation() {
  helper_Confirm_valid_Pool_Creation((MEDIUM_BLOCK_SIZE - 1),
                                     MEDIUM_BLOCK_SIZE);
  helper_Confirm_valid_Pool_Creation((MEDIUM_BLOCK_SIZE), MEDIUM_BLOCK_SIZE);
}
void test_Large_Block_Group_Allocation() {
  helper_Confirm_valid_Pool_Creation((LARGE_BLOCK_SIZE - 1), LARGE_BLOCK_SIZE);
  helper_Confirm_valid_Pool_Creation((LARGE_BLOCK_SIZE), LARGE_BLOCK_SIZE);
}
// Reallocation test group
void test_Small_reallocation() { helper_freeblock_system(SMALL_BLOCK_SIZE); }
void test_Medium_reallocation() { helper_freeblock_system(MEDIUM_BLOCK_SIZE); }
void test_Large_reallocation() { helper_freeblock_system(LARGE_BLOCK_SIZE); }
// Exhaustion test group
void test_small_exhaution() {
  helper_Small_Pool_Exhaustion(SMALL_BLOCK_SIZE, Small_Pool_Size);
}
void test_medium_exhaution() {
  helper_Small_Pool_Exhaustion(MEDIUM_BLOCK_SIZE, Medium_Pool_Size);
}
void test_large_exhaution() {
  helper_Small_Pool_Exhaustion(LARGE_BLOCK_SIZE, Large_Pool_Size);
}
// Double free test group
void test_small_double_free() { helper_double_free(SMALL_BLOCK_SIZE); }
void test_medium_double_free() { helper_double_free(MEDIUM_BLOCK_SIZE); }
void test_large_double_free() { helper_double_free(LARGE_BLOCK_SIZE); }
void test_Interleaved_Small_Allocation_And_Free() {
  helper_Interleaved_Allocation_And_Free(SMALL_BLOCK_SIZE);
}
void test_Interleaved_Medium_Allocation_And_Free() {
  helper_Interleaved_Allocation_And_Free(MEDIUM_BLOCK_SIZE);
}
void test_Interleaved_Large_Allocation_And_Free() {
  helper_Interleaved_Allocation_And_Free(LARGE_BLOCK_SIZE);
}
// Test to see if i can free a pointer that doesnt belong to my pool
void test_free_invalid_pointer_does_not_corrupt_list() {
  int stack_variable = 100;
  void *invalid_pointer = (void *)&stack_variable;

  Pool_Free(invalid_pointer, Memory_Handler); // Passes if this does nothing

  void *pointer = Pool_Alloc(10, Memory_Handler);
  TEST_ASSERT_NOT_NULL(pointer);
  Pool_Free(pointer, Memory_Handler);
}
// Freeing NULL Pointer
void test_Freeing_Null_Pointer() { Pool_Free(NULL, Memory_Handler); }
// HELPER FUNCTIONS
void helper_Small_Pool_Exhaustion(size_t Block_Size, size_t Pool_Size) {
  // see what happens if you pull more than the max pool ini values
  // loop pool allocation until poolsize +1
  void *Block_Adress_Pointer[Pool_Size];
  for (size_t i = 0; i < Pool_Size; i++) {
    Block_Adress_Pointer[i] = Pool_Alloc(Block_Size, Memory_Handler);
    TEST_ASSERT_NOT_NULL_MESSAGE(
        Block_Adress_Pointer[i],
        "Allocation failed unexpectedly before pool was full.");
  }

  TEST_ASSERT_NULL_MESSAGE(
      Pool_Alloc(Block_Size, Memory_Handler),
      "Allocation succeeded unexpectedly when pool should be full.");

  for (int i = 0; i < (Pool_Size); ++i) {
    Pool_Free(Block_Adress_Pointer[i], Memory_Handler);
  }
}

void helper_Confirm_valid_Pool_Creation(size_t Memory_Size, size_t Block_Size) {
  // Get memory allocations
  void *Block_Adress_Pointer = Pool_Alloc(Memory_Size, Memory_Handler);
  TEST_ASSERT_NOT_NULL_MESSAGE(
      Block_Adress_Pointer,
      "Allocation failed unexpectedly before pool was full.");

  // setting chunks to read in as 1 byte
  uint8_t *Memory_Block_Chunk = (uint8_t *)Block_Adress_Pointer;

  // setting each byte to 0xAA then confirming
  memset(Memory_Block_Chunk, TEST_PATTERN, Block_Size);
  for (size_t i = 0; i < Block_Size; i++) {
    TEST_ASSERT_EQUAL_UINT8(TEST_PATTERN, Memory_Block_Chunk[i]);
  }
  // free up block
  Pool_Free(Block_Adress_Pointer, Memory_Handler);
}

void helper_freeblock_system(size_t Memory_Size) {
  // get block and remember adress location
  void *Block_Adress_Pointer = Pool_Alloc(Memory_Size, Memory_Handler);
  TEST_ASSERT_NOT_NULL(Block_Adress_Pointer);
  void *MemoryAddressTempStorage = Block_Adress_Pointer;
  Pool_Free(Block_Adress_Pointer, Memory_Handler);

  // try again and compare values
  Block_Adress_Pointer = Pool_Alloc(Memory_Size, Memory_Handler);
  TEST_ASSERT_NOT_NULL(Block_Adress_Pointer);
  TEST_ASSERT_EQUAL_PTR(MemoryAddressTempStorage, Block_Adress_Pointer);
  Pool_Free(Block_Adress_Pointer, Memory_Handler);
}

void helper_double_free(size_t Memory_Size) {
  // allocate 1 of set size
  void *Block_Adress_Pointer = Pool_Alloc(Memory_Size, Memory_Handler);
  // try to free the same one twice
  Pool_Free(Block_Adress_Pointer, Memory_Handler);
  Pool_Free(Block_Adress_Pointer, Memory_Handler);
}

// Return pointer in middle and confirm it is added at front of list
void helper_Interleaved_Allocation_And_Free(size_t Memory_Size) {
  void *Block_Adress_Pointer_1 = Pool_Alloc(Memory_Size, Memory_Handler);
  void *Block_Adress_Pointer_2 = Pool_Alloc(Memory_Size, Memory_Handler);
  void *Block_Adress_Pointer_3 = Pool_Alloc(Memory_Size, Memory_Handler);
  Pool_Free(Block_Adress_Pointer_2, Memory_Handler);

  void *Block_Adress_Pointer_4 = Pool_Alloc(Memory_Size, Memory_Handler);
  TEST_ASSERT_EQUAL_PTR(Block_Adress_Pointer_2, Block_Adress_Pointer_4);
  Pool_Free(Block_Adress_Pointer_1, Memory_Handler);
  Pool_Free(Block_Adress_Pointer_3, Memory_Handler);
  Pool_Free(Block_Adress_Pointer_4, Memory_Handler);
}

#endif
