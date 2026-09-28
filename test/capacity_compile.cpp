// Compile-only check: TEST_CAPACITY=1 must compile; 0 and -1 must be rejected.
#include "../avl_array.h"
#ifndef TEST_CAPACITY
#define TEST_CAPACITY 1
#endif
avl_array<int, int, int, TEST_CAPACITY> capacity_probe;
