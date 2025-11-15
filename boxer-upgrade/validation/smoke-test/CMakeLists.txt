cmake_minimum_required(VERSION 3.16)
project(BoxerSmokeTest)

# Require C++17 or later (DOSBox Staging uses C++20, but C++17 is sufficient for our stub)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define BOXER_INTEGRATED so we can see the hooks
add_compile_definitions(BOXER_INTEGRATED=1)

# Path to DOSBox Staging source
set(DOSBOX_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../src/dosbox-staging")

# Check if DOSBox has been configured with BOXER_INTEGRATED
set(DOSBOX_BUILD_DIR "${DOSBOX_SRC_DIR}")

if(NOT EXISTS "${DOSBOX_BUILD_DIR}/CMakeCache.txt")
    message(STATUS "DOSBox has not been configured yet. Configuring with BOXER_INTEGRATED=ON...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -S ${DOSBOX_SRC_DIR} -B ${DOSBOX_BUILD_DIR}
                -DBOXER_INTEGRATED=ON
                -DCMAKE_BUILD_TYPE=Debug
        RESULT_VARIABLE CONFIG_RESULT
    )
    if(NOT CONFIG_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to configure DOSBox with BOXER_INTEGRATED=ON")
    endif()
endif()

# Check if libdosbox.a exists, if not, build it
set(DOSBOX_LIB "${DOSBOX_BUILD_DIR}/libdosbox.a")
if(NOT EXISTS "${DOSBOX_LIB}")
    message(STATUS "libdosbox.a not found. Building DOSBox library...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ${DOSBOX_BUILD_DIR} --target dosbox
        RESULT_VARIABLE BUILD_RESULT
    )
    if(NOT BUILD_RESULT EQUAL 0)
        message(WARNING "Failed to build DOSBox library automatically. Please build manually:")
        message(WARNING "  cd ${DOSBOX_BUILD_DIR}")
        message(WARNING "  cmake --build . --target dosbox")
        message(WARNING "Continuing anyway to test linkage...")
    endif()
endif()

# Create smoke test executable
add_executable(boxer-smoke-test main.cpp)

# Include DOSBox headers
target_include_directories(boxer-smoke-test PRIVATE
    ${DOSBOX_SRC_DIR}/include
    ${DOSBOX_SRC_DIR}/src
)

# Link against the DOSBox library if it exists
# Note: This may fail if dependencies aren't installed, but we want to see the attempt
if(EXISTS "${DOSBOX_LIB}")
    target_link_libraries(boxer-smoke-test PRIVATE ${DOSBOX_LIB})
    message(STATUS "Linking against: ${DOSBOX_LIB}")
else()
    message(WARNING "DOSBox library not found at: ${DOSBOX_LIB}")
    message(WARNING "The smoke test will likely fail to link.")
    message(WARNING "To build manually:")
    message(WARNING "  1. cd ${DOSBOX_SRC_DIR}")
    message(WARNING "  2. cmake -S . -B . -DBOXER_INTEGRATED=ON")
    message(WARNING "  3. cmake --build . --target dosbox")
    message(WARNING "  4. Then rebuild this smoke test")
endif()

# Print helpful information
message(STATUS "===============================================")
message(STATUS "Boxer Smoke Test Configuration")
message(STATUS "===============================================")
message(STATUS "DOSBox source: ${DOSBOX_SRC_DIR}")
message(STATUS "DOSBox build: ${DOSBOX_BUILD_DIR}")
message(STATUS "DOSBox library: ${DOSBOX_LIB}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "===============================================")
