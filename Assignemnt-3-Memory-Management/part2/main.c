#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Declaration of the custom memory functions (assume these are implemented)
#include "mmu.h"

#define SMALL_ALLOC_SIZE 16
#define LARGE_ALLOC_SIZE (1024 * 1024) // 1 MB
#define NUM_TESTS 10000

void stress_test_allocations()
{
    printf("Starting stress test...\n");

    // Array of pointers to track allocations
    void *allocations[NUM_TESTS];

    // Step 1: Perform a series of allocations
    for (int i = 0; i < NUM_TESTS; i++)
    {
        if (i % 3 == 0)
        {
            // Small allocation every third iteration
            allocations[i] = my_malloc(SMALL_ALLOC_SIZE);
        }
        else if (i % 3 == 1)
        {
            // Large allocation every second iteration
            allocations[i] = my_malloc(LARGE_ALLOC_SIZE);
        }
        else
        {
            // Zero-sized allocation to test edge case
            allocations[i] = my_malloc(0);
        }

        // Assert that all allocations are valid (NULL is acceptable for size 0)
        assert(allocations[i] != NULL || i % 3 == 2);
    }

    // Step 2: Perform deallocations
    for (int i = 0; i < NUM_TESTS; i++)
    {
        my_free(allocations[i]); // Free each allocated block
    }

    printf("Stress test passed for malloc!\n");
}

void stress_test_calloc()
{

    // Array of pointers to track allocations
    void *allocations[NUM_TESTS];

    // Step 1: Perform a series of calloc allocations
    for (int i = 0; i < NUM_TESTS; i++)
    {
        if (i % 3 == 0)
        {
            // Small allocation (array of 4 elements)
            allocations[i] = my_calloc(4, SMALL_ALLOC_SIZE);
        }
        else if (i % 3 == 1)
        {
            // Large allocation (array of 256K elements)
            allocations[i] = my_calloc(256 * 1024, sizeof(int));
        }
        else
        {
            // Zero-sized allocation to test edge case
            allocations[i] = my_calloc(0, SMALL_ALLOC_SIZE);
        }

        // Assert that all allocations are valid (NULL is acceptable for size 0)
        assert(allocations[i] != NULL || i % 3 == 2);
    }

    // Step 2: Perform deallocations
    for (int i = 0; i < NUM_TESTS; i++)
    {
        my_free(allocations[i]); // Free each allocated block
    }

    printf("Stress test passed for calloc!\n");
}

// Test suite runner
int main()
{
    stress_test_allocations(); // Stress test for malloc
    stress_test_calloc();      // Stress test for calloc
    void * a = my_malloc(10);
    my_free(a);

    printf("All stress tests passed!\n");
    return 0;
}
