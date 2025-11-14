#!/bin/bash
# validation/link-test.sh
# Test that smoke test can link against library

echo "Testing library linkage..."

# Build DOSBox library first
cd build/dosbox-staging-boxer
cmake --build . 2>&1 | tail -50
if [ $? -ne 0 ]; then
    echo "✗ Library build failed"
    exit 1
fi
echo "✓ Library built"

# Find the library
LIB=$(find . -name "libdosbox*.a" -o -name "dosbox*.lib" | head -1)
if [ -z "$LIB" ]; then
    echo "✗ Library not found"
    exit 1
fi
echo "✓ Found library: $LIB"

# Build smoke test
cd ../../validation/smoke-test
cmake . -DDOSBOX_LIB="../../build/dosbox-staging-boxer/$LIB" 2>&1 | tail -20
cmake --build . 2>&1 | tail -50
if [ $? -ne 0 ]; then
    echo "✗ Smoke test build failed"
    exit 1
fi
echo "✓ Smoke test built"

# Run smoke test
./smoke-test
if [ $? -ne 0 ]; then
    echo "✗ Smoke test failed"
    exit 1
fi
echo "✓ Smoke test passed"

echo "Link test: PASS"
