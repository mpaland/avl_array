// Compile-only check: a capacity (TEST_CAPACITY) of 1 must compile, 0 and -1 must be rejected.
// TEST_CONTAINER selects the container: avl_array (default) or avl_array2.
#include "../avl_array.h"
#include "../avl_array2.h"
#ifndef TEST_CAPACITY
#define TEST_CAPACITY 1
#endif
#ifndef TEST_CONTAINER
#define TEST_CONTAINER avl_array
#endif
TEST_CONTAINER<int, int, int, TEST_CAPACITY> capacity_probe;
