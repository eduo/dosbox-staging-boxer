#!/bin/bash
# Build and run the lifecycle smoke test
# Phase 2, Task 2-4

set -e

echo "Building lifecycle smoke test..."
c++ -std=c++20 \
    -DBOXER_INTEGRATED=1 \
    -I../../src/dosbox-staging/include \
    -o lifecycle-smoke-test \
    lifecycle-smoke-test.cpp

echo "Build successful!"
echo ""
echo "Running lifecycle smoke test..."
echo "================================"
./lifecycle-smoke-test
