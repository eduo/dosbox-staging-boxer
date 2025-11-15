cmake_minimum_required(VERSION 3.15)
project(boxer-performance-test CXX)

# C++20 required (matches DOSBox Staging requirement)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the boxer hooks headers
# Adjust path based on where this test is run from
set(DOSBOX_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../src/dosbox-staging/include")

# Performance test executable
add_executable(performance-test performance-test.cpp)

# Include directories
target_include_directories(performance-test PRIVATE ${DOSBOX_INCLUDE_DIR})

# Enable threading support (required for multi-threaded tests)
find_package(Threads REQUIRED)
target_link_libraries(performance-test PRIVATE Threads::Threads)

# Compiler optimizations for realistic performance testing
target_compile_options(performance-test PRIVATE
    # Optimization level
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-O3>
    $<$<CXX_COMPILER_ID:MSVC>:/O2>

    # Enable warnings
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Build instructions message
message(STATUS "")
message(STATUS "Boxer Performance Test Suite")
message(STATUS "============================")
message(STATUS "")
message(STATUS "To build:")
message(STATUS "  cd ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "  cmake ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "  make")
message(STATUS "")
message(STATUS "To run:")
message(STATUS "  ./performance-test")
message(STATUS "")
message(STATUS "Requirements:")
message(STATUS "  - < 1μs per call for runLoopShouldContinue")
message(STATUS "  - < 1% total overhead")
message(STATUS "  - < 100ms abort latency")
message(STATUS "")
