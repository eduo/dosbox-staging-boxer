# Validation Gates and Scripts

This document defines all validation gates that must pass before advancing phases, plus the scripts agents use to validate their work.

---

## GATE OVERVIEW

| Gate | When | What | Automated |
|------|------|------|-----------|
| Gate 0 | Pre-Phase | Prerequisites checked | Partial |
| Gate 1 | Per-Task | Static analysis | Yes |
| Gate 2 | Per-Phase | Consistency check | Partial |
| Gate 3 | Per-Phase | Human review | No |

---

## GATE 0: Pre-Phase Checklist

**Purpose**: Ensure phase can start with all dependencies met.

### Checklist Template

```markdown
## Phase [X] Pre-Start Checklist

### Dependencies
- [ ] Previous phase complete and signed off
- [ ] All blocking decisions resolved (check DECISION_LOG.md)
- [ ] Required input documents identified and available

### Resources
- [ ] Estimated hours approved
- [ ] No conflicting parallel tasks
- [ ] Human reviewer available for escalations

### Environment
- [ ] Source repos at correct branches
- [ ] Build environment functional
- [ ] Validation scripts accessible

### Documentation
- [ ] Phase objectives documented in progress/phase-X/OBJECTIVES.md
- [ ] Success criteria defined
- [ ] Task breakdown created
```

### Validation Command
```bash
#!/bin/bash
# validation/gate0-check.sh
# Check if phase can start

PHASE=$1
if [ -z "$PHASE" ]; then
    echo "Usage: gate0-check.sh <phase-number>"
    exit 1
fi

echo "Checking Gate 0 for Phase $PHASE..."

# Check previous phase complete
if [ "$PHASE" -gt 1 ]; then
    PREV=$((PHASE - 1))
    if [ ! -f "progress/phase-$PREV/PHASE_COMPLETE.md" ]; then
        echo "✗ FAIL: Phase $PREV not complete"
        exit 1
    fi
    echo "✓ Phase $PREV complete"
fi

# Check objectives documented
if [ ! -f "progress/phase-$PHASE/OBJECTIVES.md" ]; then
    echo "✗ FAIL: Objectives not documented"
    exit 1
fi
echo "✓ Objectives documented"

# Check for blocking decisions
BLOCKING=$(grep -l "PENDING.*Phase $PHASE" DECISION_LOG.md 2>/dev/null | head -1)
if [ -n "$BLOCKING" ]; then
    echo "⚠ WARNING: Blocking decisions pending for Phase $PHASE"
    grep -B5 "PENDING.*Phase $PHASE" DECISION_LOG.md | head -20
fi

# Check source repos
for repo in src/boxer src/dosbox-staging; do
    if [ ! -d "$repo/.git" ]; then
        echo "✗ FAIL: $repo not a git repo"
        exit 1
    fi
done
echo "✓ Source repositories present"

echo "Gate 0: PASS - Phase $PHASE can start"
```

---

## GATE 1: Static Analysis

**Purpose**: Validate code quality without compilation.

### What Gets Checked
1. Syntax correctness (clang -fsyntax-only)
2. Include guards present
3. #ifdef blocks balanced
4. Naming conventions followed
5. No circular dependencies

### Static Analysis Script

```bash
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
```

### Common Failures and Fixes

**Missing include guard**:

```cpp
// BAD
#pragma once  // Don't use in cross-platform code

// GOOD
#ifndef BOXER_HOOKS_H
#define BOXER_HOOKS_H
...
#endif // BOXER_HOOKS_H
```

**Unbalanced #ifdef**:

```cpp
// BAD - missing endif
#ifdef BOXER_INTEGRATED
void foo() {}
// forgot #endif!

// GOOD
#ifdef BOXER_INTEGRATED
void foo() {}
#endif
```

**Wrong naming**:

```cpp
// BAD - should be camelCase
virtual void ShellWillStart() = 0;

// GOOD
virtual void shellWillStart() = 0;
```

---

## GATE 2: Consistency Check

**Purpose**: Ensure interfaces match implementations, no orphaned code.

### What Gets Checked
1. Every declared hook has implementation (or justified TODO)
2. Every implementation references declared interface
3. No circular dependencies
4. All Category C modifications documented

### Consistency Check Script

```bash
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
```

### Common Failures and Fixes

**Missing implementation**:

```cpp
// Header declares:
virtual void shellWillStart() = 0;

// But nowhere is BOXER_HOOK_VOID(shellWillStart) called
// FIX: Add hook call in shell.cpp or document why it's not used
```

**Untracked TODO**:

```cpp
// BAD
// TODO: implement this

// GOOD
// TODO: TASK-1-5, Agent: Builder, Date: 2025-11-15
// Implement proper error handling for mount failure
```

---

## GATE 3: Human Review

**Purpose**: Ensure human oversight of critical decisions.

### Review Checklist
```markdown
## Phase [X] Human Review

### Code Review
- [ ] All Category C modifications reviewed
- [ ] No obvious performance issues
- [ ] Error handling adequate
- [ ] Thread safety considered

### Architecture Review
- [ ] Patterns match analysis recommendations
- [ ] No unnecessary complexity
- [ ] Maintains upstream compatibility

### Decision Review
- [ ] All deferred decisions listed
- [ ] Options clearly presented
- [ ] Recommendations justified

### Risk Assessment
- [ ] New risks identified
- [ ] Existing risks updated
- [ ] Mitigation strategies current

### Sign-off
- [ ] Phase deliverables complete
- [ ] Ready for next phase
- [ ] No blocking issues remain

Reviewed by: [Name]
Date: [ISO date]
Approved: [YES/NO]
Comments: [Any notes]
```

---

## BUILD VALIDATION

### CMake Configuration Test

```bash
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
```

### Link Test

```bash
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
```

---

## AUTOMATED VALIDATION RUNNER

```bash
#!/bin/bash
# validation/run-all-gates.sh
# Run all validation gates

PHASE=$1
if [ -z "$PHASE" ]; then
    echo "Usage: run-all-gates.sh <phase-number>"
    exit 1
fi

echo "========================================="
echo "Running Validation Gates for Phase $PHASE"
echo "========================================="

# Gate 0: Pre-Phase
echo -e "\n--- GATE 0: Pre-Phase Checklist ---"
./validation/gate0-check.sh $PHASE
GATE0=$?

# Gate 1: Static Analysis
echo -e "\n--- GATE 1: Static Analysis ---"
./validation/static-checks.sh src/dosbox-staging
GATE1=$?

# Gate 2: Consistency
echo -e "\n--- GATE 2: Consistency Check ---"
./validation/consistency-check.sh
GATE2=$?

# Gate 3: Human Review (manual)
echo -e "\n--- GATE 3: Human Review ---"
if [ -f "progress/phase-$PHASE/HUMAN_REVIEW.md" ]; then
    APPROVED=$(grep "Approved: YES" "progress/phase-$PHASE/HUMAN_REVIEW.md")
    if [ -n "$APPROVED" ]; then
        echo "✓ Human review approved"
        GATE3=0
    else
        echo "⚠ Human review pending or not approved"
        GATE3=1
    fi
else
    echo "⚠ Human review not yet conducted"
    GATE3=1
fi

# Summary
echo -e "\n========================================="
echo "VALIDATION SUMMARY FOR PHASE $PHASE"
echo "========================================="
echo "Gate 0 (Pre-Phase): $([ $GATE0 -eq 0 ] && echo PASS || echo FAIL)"
echo "Gate 1 (Static):    $([ $GATE1 -eq 0 ] && echo PASS || echo FAIL)"
echo "Gate 2 (Consistency): $([ $GATE2 -eq 0 ] && echo PASS || echo FAIL)"
echo "Gate 3 (Human):     $([ $GATE3 -eq 0 ] && echo PASS || echo PENDING)"

if [ $GATE0 -eq 0 ] && [ $GATE1 -eq 0 ] && [ $GATE2 -eq 0 ] && [ $GATE3 -eq 0 ]; then
    echo -e "\n✓ ALL GATES PASS - Ready to advance to Phase $((PHASE + 1))"
    exit 0
else
    echo -e "\n✗ GATES NOT ALL PASSED - Cannot advance"
    exit 1
fi
```

---

## USING VALIDATION IN AGENT TASKS

Every agent task should include:

```markdown
### Validation Commands
Run these YOURSELF before reporting completion:

# 1. Immediate validation (after writing code)
./validation/static-checks.sh src/dosbox-staging

# 2. If modifying CMake
./validation/build-test.sh

# 3. If adding hooks
./validation/consistency-check.sh

# 4. Report results in task report
```

Agents should:
1. Run validation BEFORE reporting task complete
2. Fix all Gate 1 errors before reporting
3. Report Gate 2 issues (may require human input)
4. Note any concerns for Gate 3 review

---

## VALIDATION METRICS

Track these over time:

- **Gate 1 pass rate**: Should be 100% (agents fix before reporting)
- **Gate 2 first-pass rate**: Target >80% (some issues expected)
- **Gate 3 approval rate**: Target >90% (human rarely rejects)
- **Time spent in validation**: Target <15% of phase time

Log these in PROGRESS.md after each phase.
