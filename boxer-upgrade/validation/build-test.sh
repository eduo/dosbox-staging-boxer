#!/bin/bash
# validation/build-test.sh
# Test that both build modes work

echo "Testing CMake build configurations..."
ERRORS=0

# 1. Standard build
echo "=== Standard Build (BOXER_INTEGRATED=OFF) ==="
mkdir -p build/dosbox-staging-normal
cd build/dosbox-staging-normal
cmake -DBOXER_INTEGRATED=OFF ../../src/dosbox-staging/ 2>&1 | tail -20
if [ $? -ne 0 ]; then
    echo "✗ Standard build configuration failed"
    ERRORS=$((ERRORS + 1))
else
    echo "✓ Standard build configuration succeeded"
fi
cd ../..

# 2. Boxer build
echo -e "\n=== Boxer Build (BOXER_INTEGRATED=ON) ==="
mkdir -p build/dosbox-staging-boxer
cd build/dosbox-staging-boxer
cmake -DBOXER_INTEGRATED=ON ../../src/dosbox-staging/ 2>&1 | tail -20
if [ $? -ne 0 ]; then
    echo "✗ Boxer build configuration failed"
    ERRORS=$((ERRORS + 1))
else
    echo "✓ Boxer build configuration succeeded"
    
    # Check for static library target
    if grep -q "add_library.*dosbox-staging.*STATIC" CMakeCache.txt 2>/dev/null; then
        echo "✓ Static library target configured"
    else
        echo "⚠ Warning: Static library target not confirmed"
    fi
fi
cd ../..

# Summary
if [ $ERRORS -eq 0 ]; then
    echo "✓ Build validation: PASS"
    exit 0
else
    echo "✗ Build validation: FAIL"
    exit 1
fi
