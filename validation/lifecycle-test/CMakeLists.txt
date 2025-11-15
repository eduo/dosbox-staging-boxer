# Lifecycle Test Suite for Boxer-DOSBox Integration
# Tests INT-057, INT-058, INT-059 lifecycle hooks

cmake_minimum_required(VERSION 3.16)
project(BoxerLifecycleTest CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include boxer hooks headers
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/dosbox-staging/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/dosbox-staging/include/boxer
)

# Build lifecycle test executable
add_executable(lifecycle-test
    lifecycle-test.cpp
)

# Thread support required for multi-threaded abort test
find_package(Threads REQUIRED)
target_link_libraries(lifecycle-test Threads::Threads)

# Enable BOXER_INTEGRATED to activate hooks
target_compile_definitions(lifecycle-test PRIVATE BOXER_INTEGRATED)

# Output location
set_target_properties(lifecycle-test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

message(STATUS "Configured Boxer Lifecycle Test Suite")
message(STATUS "  Build with: cmake --build .")
message(STATUS "  Run with: ./lifecycle-test")
