
project(cppalloc_general_tests)

include(ExternalProject)

ExternalProject_Add(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG origin/v2.x
        PREFIX ${CMAKE_SOURCE_DIR}/external/Catch2
        STEP_TARGETS install
        CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_SOURCE_DIR}/out/external/install
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_CONFIGURATION_TYPES=${CMAKE_CONFIGURATION_TYPES}
        -DCATCH_BUILD_TESTING=OFF
        )

## Basic tests
macro(validity_test test_name definitions compile_flags link_options)
    add_executable(cppalloc-unit-test-validity-${test_name}
            "validity/linear_allocator.cpp"
            "validity/main.cpp"
            "validity/pool_allocator.cpp"
            "validity/arena_allocator.cpp"
            )
    target_link_libraries(cppalloc-unit-test-validity-${test_name} cppalloc)
    add_test(validity-${test_name} cppalloc-unit-test-validity-${test_name})
    add_dependencies(cppalloc-unit-test-validity-${test_name} Catch2-install)
    target_include_directories(cppalloc-unit-test-validity-${test_name} PRIVATE "${CMAKE_SOURCE_DIR}/out/external/install/include")
    target_include_directories(cppalloc-unit-test-validity-${test_name} PRIVATE "${CMAKE_SOURCE_DIR}/include")
    target_compile_definitions(cppalloc-unit-test-validity-${test_name} PRIVATE ${definitions})
    target_compile_options(cppalloc-unit-test-validity-${test_name} PRIVATE ${compile_flags})
    target_compile_features(cppalloc-unit-test-validity-${test_name} PRIVATE cxx_std_17)
    target_link_options(cppalloc-unit-test-validity-${test_name} PRIVATE ${link_options})
endmacro()

set(CPPALLOC_COMMON_CXX_FLAGS "")
set(CPPALLOC_COMMON_CXX_LINK_FLAGS "")

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CPPALLOC_COMMON_CXX_FLAGS "-fsanitize=address;-fno-omit-frame-pointer;-ftest-coverage;-fprofile-instr-generate;-fcoverage-mapping")
    set(CPPALLOC_COMMON_CXX_LINK_FLAGS "-fsanitize=address;-fprofile-instr-generate")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CPPALLOC_COMMON_CXX_FLAGS "-fsanitize=address;-fno-omit-frame-pointer;-ftest-coverage")
    set(CPPALLOC_COMMON_CXX_LINK_FLAGS "-fsanitize=address")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    set(CPPALLOC_COMMON_CXX_FLAGS "")
    set(CPPALLOC_COMMON_CXX_LINK_FLAGS "")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CPPALLOC_COMMON_CXX_FLAGS "")
    set(CPPALLOC_COMMON_CXX_LINK_FLAGS "")
endif ()

validity_test("cpp" "" "${CPPALLOC_COMMON_CXX_FLAGS}" "${CPPALLOC_COMMON_CXX_LINK_FLAGS}")
