#!/bin/bash
# validation/consistency-check.sh
# Check interface-implementation consistency

echo "Running consistency checks..."
ERRORS=0

# 1. Count hook declarations vs implementations
echo "=== Hook Declaration/Implementation Match ==="
HEADER_FILE="src/dosbox-staging/include/boxer/boxer_hooks.h"
IMPL_FILES="src/dosbox-staging src/boxer"

if [ -f "$HEADER_FILE" ]; then
    DECLARED=$(grep "virtual.*) = 0;" "$HEADER_FILE" | wc -l)
    echo "Declared in IBoxerDelegate: $DECLARED hooks"
    
    # Check that macros exist for each hook
    MACROS=$(grep "BOXER_HOOK" "$HEADER_FILE" | grep "#define" | wc -l)
    echo "Hook macros defined: $MACROS"
else
    echo "✗ Hook header not found"
    ERRORS=$((ERRORS + 1))
fi

# 2. Check for orphaned BOXER_HOOK calls
echo -e "\n=== Orphaned BOXER_HOOK Calls ==="
for call in $(grep -rh "BOXER_HOOK_.*(" $IMPL_FILES --include="*.cpp" 2>/dev/null | \
              sed 's/.*BOXER_HOOK_[A-Z_]*(\([^,)]*\).*/\1/' | sort | uniq); do
    if ! grep -q "virtual.*$call" "$HEADER_FILE" 2>/dev/null; then
        echo "✗ BOXER_HOOK call to '$call' has no declaration"
        ERRORS=$((ERRORS + 1))
    fi
done

if [ $ERRORS -eq 0 ]; then
    echo "✓ No orphaned hook calls"
fi

# 3. Category C modification tracking
echo -e "\n=== Category C Modifications ==="
EXPECTED_MODS=14
FOUND_MODS=$(grep -r "#ifdef BOXER_INTEGRATED" src/dosbox-staging/src --include="*.cpp" | wc -l)
echo "Expected Category C modifications: $EXPECTED_MODS"
echo "Found #ifdef BOXER_INTEGRATED in source: $FOUND_MODS files"

if [ "$FOUND_MODS" -lt "$EXPECTED_MODS" ]; then
    echo "⚠ Not all Category C modifications implemented yet"
fi

# 4. TODOs with tracking
echo -e "\n=== Untracked TODOs ==="
UNTRACKED=$(grep -r "TODO[^:]" src/dosbox-staging --include="*.cpp" --include="*.h" 2>/dev/null | \
            grep -v "Agent\|Date\|TASK" | wc -l)
if [ "$UNTRACKED" -gt 0 ]; then
    echo "⚠ $UNTRACKED TODOs without agent/task tracking"
    grep -r "TODO[^:]" src/dosbox-staging --include="*.cpp" --include="*.h" 2>/dev/null | \
        grep -v "Agent\|Date\|TASK" | head -5
fi

# Summary
echo -e "\n=== SUMMARY ==="
if [ $ERRORS -eq 0 ]; then
    echo "✓ Gate 2: PASS - Consistency checks passed"
    exit 0
else
    echo "✗ Gate 2: FAIL - $ERRORS inconsistencies found"
    exit 1
fi
