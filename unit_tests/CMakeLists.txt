
project(cppalloc_general_tests)

## Basic tests
add_executable(test_cppalloc_general general_test.cpp)
add_test(cppalloc_general test_cppalloc_general)

target_link_libraries(test_cppalloc_general cppalloc)


