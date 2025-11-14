#!/bin/bash
# validation/static-checks.sh
# Static analysis for Boxer DOSBox integration code

TARGET_DIR="${1:-src/dosbox-staging}"
ERRORS=0

echo "Running static analysis on $TARGET_DIR..."

# 1. Syntax check all C++ files
echo "=== C++ Syntax Check ==="
for f in $(find "$TARGET_DIR" -name "*.cpp" -o -name "*.h" | grep -E "boxer|BOXER"); do
    if clang++ -std=c++17 -fsyntax-only -I"$TARGET_DIR/include" "$f" 2>&1 | head -20; then
        : # No output means success
    else
        echo "✗ Syntax error in $f"
        ERRORS=$((ERRORS + 1))
    fi
done

if [ $ERRORS -eq 0 ]; then
    echo "✓ All syntax checks passed"
else
    echo "✗ $ERRORS syntax errors found"
fi

# 2. Include guards
echo -e "\n=== Include Guard Check ==="
for f in $(find "$TARGET_DIR/include" -name "*.h" | grep boxer); do
    GUARD=$(basename "$f" | tr '[:lower:]' '[:upper:]' | sed 's/\./_/g')
    if grep -q "#ifndef.*${GUARD%%_H}_H" "$f"; then
        echo "✓ $f has proper guard"
    else
        echo "✗ $f missing or incorrect include guard"
        ERRORS=$((ERRORS + 1))
    fi
done

# 3. #ifdef balance check
echo -e "\n=== #ifdef Balance Check ==="
for f in $(find "$TARGET_DIR" \( -name "*.cpp" -o -name "*.h" \) -exec grep -l "ifdef BOXER" {} \;); do
    IFDEFS=$(grep -c "#ifdef BOXER" "$f")
    ENDIFS=$(grep -c "#endif.*BOXER\|#endif$" "$f")
    if [ "$IFDEFS" -gt "$ENDIFS" ]; then
        echo "✗ $f: Unbalanced #ifdef BOXER ($IFDEFS opens, $ENDIFS closes)"
        ERRORS=$((ERRORS + 1))
    else
        echo "✓ $f: #ifdef blocks balanced"
    fi
done

# 4. Naming convention check
echo -e "\n=== Naming Convention Check ==="
for f in $(find "$TARGET_DIR" -name "*.h" | grep boxer); do
    # Hook functions should start with boxer_ or be camelCase methods
    BAD_NAMES=$(grep "virtual.*\s\+[A-Z]" "$f" | grep -v "//")
    if [ -n "$BAD_NAMES" ]; then
        echo "⚠ $f: Possible naming issues:"
        echo "$BAD_NAMES"
    fi
done

# 5. TODO tracking
echo -e "\n=== TODO Tracking ==="
TODOS=$(grep -r "TODO" "$TARGET_DIR" --include="*.cpp" --include="*.h" | grep boxer | wc -l)
echo "Found $TODOS TODOs in Boxer-related code"
if [ "$TODOS" -gt 0 ]; then
    grep -r "TODO" "$TARGET_DIR" --include="*.cpp" --include="*.h" | grep boxer | head -10
fi

# Summary
echo -e "\n=== SUMMARY ==="
if [ $ERRORS -eq 0 ]; then
    echo "✓ Gate 1: PASS - All static checks passed"
    exit 0
else
    echo "✗ Gate 1: FAIL - $ERRORS errors found"
    exit 1
fi
