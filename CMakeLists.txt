cmake_minimum_required(VERSION 3.15)   # 3.15: MSVC warning flags are not preset

project(avl_array LANGUAGES CXX)

# build the tests by default only if this is the top level project
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(AVL_ARRAY_IS_TOP_LEVEL ON)
else()
  set(AVL_ARRAY_IS_TOP_LEVEL OFF)
endif()

option(AVL_ARRAY_BUILD_TESTS "Build the test suite and the benchmark" ${AVL_ARRAY_IS_TOP_LEVEL})


# the header only library (avl_array.h and avl_array2.h)
add_library(avl_array INTERFACE)
target_include_directories(avl_array INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})


# the tests
if(AVL_ARRAY_BUILD_TESTS)
  enable_testing()

  if(MSVC)
    set(AVL_ARRAY_WARNINGS /W4 /permissive-)
  else()
    set(AVL_ARRAY_WARNINGS -Wall -Wextra -pedantic -Wsign-conversion -Wshadow)
  endif()

  # C++11 with the warnings above for all test programs
  function(avl_array_test_target target)
    target_link_libraries(${target} PRIVATE avl_array)
    target_compile_options(${target} PRIVATE ${AVL_ARRAY_WARNINGS})
    set_target_properties(${target} PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
  endfunction()

  # the test suite of avl_array and avl_array2 (Catch v2, provides main())
  # the bundled Catch 2.11.3 doesn't compile with the POSIX signal handling of current glibc versions
  add_executable(test_suite test/test_suite.cpp)
  avl_array_test_target(test_suite)
  target_compile_definitions(test_suite PRIVATE CATCH_CONFIG_NO_POSIX_SIGNALS)
  target_compile_options(test_suite PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)   # too many sections in a Debug build
  add_test(NAME test_suite COMMAND test_suite)

  # the benchmark, run with one round as smoke test, and its minimal size driver
  add_executable(benchmark test/benchmark.cpp)
  avl_array_test_target(benchmark)
  add_test(NAME benchmark COMMAND benchmark 1)

  add_executable(benchmark_size test/benchmark.cpp)
  avl_array_test_target(benchmark_size)
  target_compile_definitions(benchmark_size PRIVATE AVL_ARRAY_SIZE_ONLY)

  # a capacity of 1 must compile, a capacity of 0 or -1 must be rejected by both containers
  foreach(container avl_array avl_array2)
    add_library(capacity_${container} OBJECT test/capacity_compile.cpp)
    avl_array_test_target(capacity_${container})
    target_compile_definitions(capacity_${container} PRIVATE TEST_CONTAINER=${container} TEST_CAPACITY=1)

    foreach(capacity 0 -1)
      string(REPLACE "-" "minus" suffix "${capacity}")
      set(target capacity_${container}_${suffix})
      add_library(${target} OBJECT EXCLUDE_FROM_ALL test/capacity_compile.cpp)
      avl_array_test_target(${target})
      target_compile_definitions(${target} PRIVATE TEST_CONTAINER=${container} TEST_CAPACITY=${capacity})
      add_test(NAME ${target} COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${target} --config $<CONFIG>)
      # passed if the build fails because of the static assertion
      set_tests_properties(${target} PROPERTIES PASS_REGULAR_EXPRESSION "capacity must be positive")
    endforeach()
  endforeach()
endif()
