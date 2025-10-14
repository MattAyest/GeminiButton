/*MemoryPool Control for esp32 
    Written by Matthew Ayestaran
    Date 04/10/2025
    purpose: this contains the unit tests for the memory control header
*/

//#ifdef UNIT_TEST
#include <unity.h>
#include <MemoryPool.h>

// Optional: A function that runs before each test
void setUp(void) {
    MemoryPoolFreeBlockInitiation(5,5,5)
}

// Optional: A function that runs after each test
void tearDown(void) {
    // clean up stuff...
}

// A test case is just a function that starts with 'test_'
void test_Pool_Allocation_Should_Return_Pointer(void) {
    // ask to allocate memorypool and confirm it is working;
    UNIT_TEST(MemoryPoolFreeBlockInitiation(5,5,5));

}

void test_addition_should_work_for_negative_numbers(void) {
    TEST_ASSERT_EQUAL_INT(-5, -2 + -3);
}

// This is the main function that runs your tests
void app_main(void) { // On ESP32, or use main() for native tests
    UNITY_BEGIN(); // Starts the test runner

    // Run your test cases
    RUN_TEST(test_addition_should_work_for_positive_numbers);
    RUN_TEST(test_addition_should_work_for_negative_numbers);
    
    UNITY_END(); // Ends the test runner and prints a summary
}
//#endif // UNIT_TEST