# Phase 1: Foundation - Agent Tasks

**Phase Duration**: Weeks 1-2
**Total Estimated Hours**: 60-80 hours
**Goal**: Establish build system and core infrastructure

---

## PHASE 1 OVERVIEW

By the end of Phase 1, you will have:
- DOSBox Staging building as a static library with BOXER_INTEGRATED=ON
- All hook infrastructure headers in place
- Stub implementations for all 86 integration points
- Basic validation that Boxer can link against the library

**This phase produces NO runtime functionality.** Everything is scaffolding.

---

## TASK 1-1: Directory and CMake Setup

### Context
- **Phase**: 1
- **Estimated Hours**: 10-14 hours
- **Criticality**: CORE
- **Risk Level**: LOW

### Objective
Add BOXER_INTEGRATED CMake option to DOSBox Staging that builds it as a static library instead of executable.

### Prerequisites
- [ ] boxer-upgrade/ directory structure created
- [ ] All three source repos cloned into src/
- [ ] Read unavoidable-modifications.md lines 34-137

### Input Documents
1. `analysis/03-reintegration-analysis/unavoidable-modifications.md` lines 34-137
   - Contains exact CMake code to add
2. `src/dosbox-staging/CMakeLists.txt`
   - Current target DOSBox build structure
3. `src/dosbox-staging/src/CMakeLists.txt`
   - Source file listing (if exists)

### Deliverables
1. **Modified**: `src/dosbox-staging/CMakeLists.txt`
   - Add BOXER_INTEGRATED option block
   - ~43 lines added
   
2. **New file**: `src/dosbox-staging/cmake/BoxerIntegration.cmake`
   - Helper module for Boxer-specific settings
   
3. **New file**: `src/dosbox-staging/include/boxer/boxer_config.h`
   - Configuration header for Boxer integration
   
4. **Documentation**: `progress/phase-1/tasks/TASK-1-1.md`
   - Task completion report

### Constraints
- **DO NOT MODIFY**: Any source code files (.cpp/.h in src/)
- **MUST PRESERVE**: Standard DOSBox build when BOXER_INTEGRATED=OFF
- **MUST USE**: CMake 3.16+ syntax (match target DOSBox)
- **GUARD ALL CHANGES**: Use `if(BOXER_INTEGRATED)` blocks

### Validation Commands
```bash
cd boxer-upgrade/

# Test 1: Standard build still configures
cd build/dosbox-staging-normal
cmake -DBOXER_INTEGRATED=OFF ../../src/dosbox-staging/
# Expected: Configuration succeeds

# Test 2: Boxer build configures with library target
cd ../dosbox-staging-boxer
cmake -DBOXER_INTEGRATED=ON ../../src/dosbox-staging/
# Expected: Configuration succeeds, shows "Building for Boxer integration"

# Test 3: Check library target exists
grep "add_library.*dosbox-staging" ../../src/dosbox-staging/CMakeLists.txt
# Expected: Found when BOXER_INTEGRATED=ON

# Test 4: SDL_MAIN_HANDLED defined
grep "SDL_MAIN_HANDLED" ../../src/dosbox-staging/CMakeLists.txt
# Expected: Present in BOXER_INTEGRATED block
```

### Decision Points - STOP if:

1. **CMake version conflict**: Target DOSBox requires CMake version incompatible with proposed syntax
   - Report: What version is required, what syntax fails

2. **Source file list structure**: DOSBOX_SOURCES variable doesn't exist or is structured differently
   - Options: A) Create new variable, B) Append to existing, C) Use generator expressions
   - Stop and report which approach

3. **Existing BOXER references**: If target already has any BOXER-related code (unlikely but check)
   - Report: What exists, potential conflicts

4. **SDL2 handling**: If target DOSBox uses find_package(SDL2) in a way that conflicts
   - Report: Current SDL2 integration, proposed changes

### Success Criteria
- [ ] `cmake -DBOXER_INTEGRATED=OFF` works exactly as before
- [ ] `cmake -DBOXER_INTEGRATED=ON` configures without errors
- [ ] BOXER_INTEGRATED=ON shows appropriate status message
- [ ] add_library() creates STATIC target
- [ ] SDL_MAIN_HANDLED is defined
- [ ] All new code inside if(BOXER_INTEGRATED) guards
- [ ] No syntax errors in CMakeLists.txt

### Report Format
File: `progress/phase-1/tasks/TASK-1-1.md`

```markdown
# TASK 1-1: Directory and CMake Setup

**Agent**: [Your ID]
**Date**: [ISO timestamp]
**Status**: COMPLETE / BLOCKED

## Work Completed
[Description of what was done]

## Files Modified
- `src/dosbox-staging/CMakeLists.txt`: +43 lines (BOXER_INTEGRATED block)
- `src/dosbox-staging/cmake/BoxerIntegration.cmake`: +[X] lines (new file)
- `src/dosbox-staging/include/boxer/boxer_config.h`: +[X] lines (new file)

## Decisions Made (Within Scope)
- [Decision]: [Justification]

## Decisions Deferred to Human
- [None / List with options]

## Validation Results
- Test 1 (Standard build): PASS/FAIL
- Test 2 (Boxer build): PASS/FAIL
- Test 3 (Library target): PASS/FAIL
- Test 4 (SDL_MAIN_HANDLED): PASS/FAIL

## Concerns Identified
- [Any risks or issues]

## Next Steps
- Proceed to TASK 1-2: Hook Infrastructure Headers
```

---

## TASK 1-2: Hook Infrastructure Headers

### Context
- **Phase**: 1
- **Estimated Hours**: 8-12 hours
- **Criticality**: CORE
- **Risk Level**: LOW

### Objective
Create the IBoxerDelegate interface and BOXER_HOOK macros that all 86 integration points will use.

### Prerequisites
- [ ] TASK 1-1 complete
- [ ] CMake configuration working with BOXER_INTEGRATED=ON
- [ ] Read integration-overview.md for all 86 hook signatures

### Input Documents
1. `analysis/01-current-integration/integration-overview.md`
   - All 86 integration points listed with types
2. `src/boxer/Boxer/BXCoalface.h`
   - Legacy hook declarations (reference only)
3. `analysis/03-reintegration-analysis/unavoidable-modifications.md`
   - Hook macro design patterns

### Deliverables
1. **New file**: `src/dosbox-staging/include/boxer/boxer_hooks.h`
   - IBoxerDelegate abstract interface
   - Global delegate pointer
   - BOXER_HOOK macros
   - All 86 method declarations
   
2. **New file**: `src/dosbox-staging/include/boxer/boxer_types.h`
   - Type definitions shared between Boxer and DOSBox
   
3. **Documentation**: `progress/phase-1/tasks/TASK-1-2.md`

### Constraints
- **MUST COMPILE STANDALONE**: Header must compile with just:
  ```bash
  clang++ -std=c++17 -fsyntax-only boxer_hooks.h
  ```
- **NO IMPLEMENTATION**: Only declarations in header
- **THREAD SAFETY**: All hook methods must be safe to call from any thread
- **NAMING**: All methods use camelCase, match legacy naming where possible

### Template for boxer_hooks.h
```cpp
#ifndef BOXER_HOOKS_H
#define BOXER_HOOKS_H

#include <cstdint>
#include <atomic>

// Forward declarations
struct SDL_Window;

// ============================================================================
// Boxer Integration Hooks - Abstract Interface
// ============================================================================

class IBoxerDelegate {
public:
    virtual ~IBoxerDelegate() = default;

    // ========================================================================
    // Emulation Lifecycle (5 points) - CRITICAL
    // ========================================================================
    
    /**
     * @brief Check if emulation loop should continue
     * @return true to continue, false to abort immediately
     * @performance MUST complete in <1μs (called ~10,000/sec)
     * @thread-safety MUST be safe from emulation thread
     * @critical This is THE emergency abort mechanism
     */
    virtual bool shouldContinueRunLoop() = 0;
    
    virtual void runLoopWillStart() = 0;
    virtual void runLoopDidFinish() = 0;
    
    // ... [Continue for all 86 hooks]
    
    // ========================================================================
    // Rendering Pipeline (15 points)
    // ========================================================================
    
    virtual void processEvents() = 0;
    virtual bool startFrame(/* params */) = 0;
    virtual void finishFrame() = 0;
    // ... etc
    
    // ========================================================================
    // Shell Integration (15 points)
    // ========================================================================
    
    virtual void shellWillStart() = 0;
    virtual void shellDidFinish(int exit_code) = 0;
    // ... etc
    
    // ========================================================================
    // [Continue for all categories]
    // ========================================================================
};

// ============================================================================
// Global Delegate Registration
// ============================================================================

// Boxer sets this at initialization, before emulation starts
extern IBoxerDelegate* g_boxer_delegate;

// ============================================================================
// Hook Invocation Macros
// ============================================================================

// For hooks that return bool (with safe default)
#define BOXER_HOOK_BOOL(name, ...) \
    (g_boxer_delegate ? g_boxer_delegate->name(__VA_ARGS__) : true)

// For hooks that return bool (must have delegate)
#define BOXER_HOOK_BOOL_REQUIRED(name, ...) \
    (g_boxer_delegate ? g_boxer_delegate->name(__VA_ARGS__) : (assert(!"Required Boxer hook called without delegate"), true))

// For hooks that return void
#define BOXER_HOOK_VOID(name, ...) \
    do { if (g_boxer_delegate) g_boxer_delegate->name(__VA_ARGS__); } while(0)

// For hooks that return value
#define BOXER_HOOK_VALUE(name, default_val, ...) \
    (g_boxer_delegate ? g_boxer_delegate->name(__VA_ARGS__) : (default_val))

#endif // BOXER_HOOKS_H
```

### Decision Points - STOP if:

1. **Signature ambiguity**: Legacy hook has unclear parameter types
   - Report: Which hook, what the ambiguity is, proposed resolution

2. **Missing hook in legacy**: Integration point documented but no declaration found
   - Report: Which point, what it should do

3. **Name collision**: Hook name conflicts with existing DOSBox function
   - Options: A) Prefix with boxer_, B) Use different name, C) Rename DOSBox function
   - Stop and report

4. **Thread safety concern**: Hook might not be safe to call from multiple threads
   - Report: Which hook, what the concern is

### Success Criteria
- [ ] boxer_hooks.h compiles standalone: `clang++ -std=c++17 -fsyntax-only`
- [ ] All 86 hooks declared in IBoxerDelegate
- [ ] Each hook has documentation comment
- [ ] All macros properly defined with safe defaults
- [ ] No implementation code (pure interface)
- [ ] No circular includes
- [ ] Guards present (#ifndef BOXER_HOOKS_H)

### Report Format
File: `progress/phase-1/tasks/TASK-1-2.md`

---

## TASK 1-3: Stub Implementations

### Context
- **Phase**: 1
- **Estimated Hours**: 6-10 hours
- **Criticality**: MEDIUM
- **Risk Level**: LOW

### Objective
Create stub implementations for DOSBox-side hooks so the library links without undefined symbols.

### Prerequisites
- [ ] TASK 1-2 complete
- [ ] boxer_hooks.h compiles
- [ ] All 86 hooks declared

### Input Documents
1. `src/dosbox-staging/include/boxer/boxer_hooks.h` (from TASK 1-2)
2. `analysis/03-reintegration-analysis/unavoidable-modifications.md`
   - Sections on each Category C modification

### Deliverables
1. **New file**: `src/dosbox-staging/src/boxer/boxer_hooks.cpp`
   - Global delegate pointer definition
   - NO implementation code (stubs only)
   
2. **Documentation**: `progress/phase-1/tasks/TASK-1-3.md`

### Template for boxer_hooks.cpp
```cpp
// ============================================================================
// FILE: src/boxer/boxer_hooks.cpp
// Stub implementation for Boxer integration hooks
// ============================================================================

#include "boxer/boxer_hooks.h"

// Global delegate pointer - set by Boxer before emulation starts
// When null, all hooks fall back to default behavior
IBoxerDelegate* g_boxer_delegate = nullptr;

// No implementation code needed here!
// All hooks go through BOXER_HOOK_* macros which check g_boxer_delegate
// Actual implementation is on the Boxer side (Objective-C++)
```

### Constraints
- **MINIMAL CODE**: Just the global pointer definition
- **NO LOGIC**: All logic is in macros or on Boxer side
- **MUST LINK**: Library must link without undefined symbols

### Validation Commands
```bash
# Test 1: Compiles
clang++ -std=c++17 -c src/dosbox-staging/src/boxer/boxer_hooks.cpp \
    -I src/dosbox-staging/include/
# Expected: No errors, produces boxer_hooks.o

# Test 2: No undefined symbols from hooks
nm boxer_hooks.o | grep g_boxer_delegate
# Expected: Shows symbol defined
```

### Success Criteria
- [ ] File compiles without errors
- [ ] g_boxer_delegate symbol defined
- [ ] No undefined symbols related to hooks
- [ ] File is minimal (<20 lines)

---

## TASK 1-4: CMake Source Integration

### Context
- **Phase**: 1
- **Estimated Hours**: 4-6 hours
- **Criticality**: CORE
- **Risk Level**: MEDIUM

### Objective
Update CMakeLists.txt to include the new Boxer source files in the build.

### Prerequisites
- [ ] TASK 1-1 complete (CMake option exists)
- [ ] TASK 1-3 complete (stub source file exists)

### Deliverables
1. **Modified**: `src/dosbox-staging/CMakeLists.txt`
   - Add src/boxer/boxer_hooks.cpp to source list
   - Add include/boxer to include path
   
2. **Documentation**: `progress/phase-1/tasks/TASK-1-4.md`

### Validation Commands
```bash
cd build/dosbox-staging-boxer
cmake -DBOXER_INTEGRATED=ON ../../src/dosbox-staging/
cmake --build .

# Expected: Library builds, includes boxer_hooks.o
```

### Success Criteria
- [ ] CMake configuration succeeds
- [ ] Build includes boxer_hooks.cpp
- [ ] Static library created (libdosbox-staging.a or similar)
- [ ] No linker errors related to hooks

---

## TASK 1-5: First Integration Point (INT-059)

### Context
- **Phase**: 1
- **Estimated Hours**: 8-12 hours
- **Criticality**: CRITICAL
- **Risk Level**: HIGH

### Objective
Add the single most critical hook: INT-059 (shouldContinueRunLoop) into the DOSBox emulation loop. This is the emergency abort mechanism.

### Prerequisites
- [ ] TASK 1-4 complete (library builds)
- [ ] Read unavoidable-modifications.md lines 857-1000
- [ ] Identify exact location of normal_loop() in target DOSBox

### Input Documents
1. `analysis/03-reintegration-analysis/unavoidable-modifications.md` lines 857-1000
   - Detailed analysis of INT-059
   - Performance requirements
   - Code samples
2. `src/dosbox-staging/src/dosbox.cpp`
   - Find normal_loop() function

### Deliverables
1. **Modified**: `src/dosbox-staging/src/dosbox.cpp`
   - Add #include "boxer/boxer_hooks.h"
   - Add BOXER_HOOK check in normal_loop()
   - ~5-6 lines added
   
2. **Documentation**: `progress/phase-1/tasks/TASK-1-5.md`

### CRITICAL: Code to Add
```cpp
// In src/dosbox.cpp, find normal_loop()
#ifdef BOXER_INTEGRATED
#include "boxer/boxer_hooks.h"
#endif

static uint32_t normal_loop() {
    while (true) {
        #ifdef BOXER_INTEGRATED
        // CRITICAL: Check if Boxer wants to abort emulation
        // Called ~10,000 times/sec, must be <1μs
        if (!BOXER_HOOK_BOOL(shouldContinueRunLoop)) {
            return 1; // Exit emulation
        }
        #endif
        
        // ... existing emulation code
    }
}
```

### Decision Points - STOP if:

1. **normal_loop() structure differs**: Function doesn't have simple while(true) structure
   - Report: Actual structure, proposed insertion point

2. **Multiple loop locations**: More than one place where loop runs
   - Report: All locations, which to modify

3. **Performance concern**: Loop runs much faster than 10,000/sec
   - Report: Estimated frequency, impact on overhead

### Validation Commands
```bash
# Test 1: Builds with modification
cd build/dosbox-staging-boxer
cmake --build .
# Expected: Compiles successfully

# Test 2: Standard build unaffected
cd ../dosbox-staging-normal
cmake --build .
# Expected: Compiles without BOXER_HOOK code

# Test 3: Symbol check
nm libdosbox-staging.a | grep shouldContinueRunLoop
# Expected: Shows undefined symbol (will be provided by Boxer)
```

### Success Criteria
- [ ] Modification is <10 lines
- [ ] All changes inside #ifdef BOXER_INTEGRATED
- [ ] Both BOXER_INTEGRATED=ON and OFF builds succeed
- [ ] Hook is in the hot path (inside while loop)
- [ ] Early exit path returns reasonable value (1)

---

## TASK 1-6: Link Test Harness

### Context
- **Phase**: 1
- **Estimated Hours**: 4-6 hours
- **Criticality**: MAJOR
- **Risk Level**: LOW

### Objective
Create minimal Objective-C++ test that links against DOSBox library and provides a stub delegate.

### Prerequisites
- [ ] TASK 1-4 complete (library builds)
- [ ] TASK 1-5 complete (INT-059 integrated)

### Deliverables
1. **New file**: `validation/smoke-test/main.mm`
   - Objective-C++ main entry point
   - Stub BoxerDelegate implementation
   - Sets g_boxer_delegate
   - Immediately signals abort
   
2. **New file**: `validation/smoke-test/CMakeLists.txt`
   - Builds smoke test executable
   - Links against libdosbox-staging.a
   
3. **Documentation**: `progress/phase-1/tasks/TASK-1-6.md`

### Template for main.mm
```objc
// Smoke test: Link against DOSBox library, verify hooks are callable

#include "boxer/boxer_hooks.h"
#include <iostream>

// Minimal stub implementation
class BoxerDelegateStub : public IBoxerDelegate {
public:
    bool shouldContinueRunLoop() override {
        // Immediately abort - we're just testing linkage
        return false;
    }
    
    void runLoopWillStart() override {}
    void runLoopDidFinish() override {}
    // ... stub all 86 methods
};

int main(int argc, char* argv[]) {
    std::cout << "Boxer DOSBox Integration Smoke Test\n";
    
    BoxerDelegateStub delegate;
    g_boxer_delegate = &delegate;
    
    // Test: Can we call hooks?
    bool should_continue = BOXER_HOOK_BOOL(shouldContinueRunLoop);
    std::cout << "shouldContinueRunLoop: " << (should_continue ? "true" : "false") << "\n";
    
    if (!should_continue) {
        std::cout << "✓ Hook works! Abort signal received.\n";
        return 0;
    } else {
        std::cout << "✗ Hook failed - expected false\n";
        return 1;
    }
}
```

### Success Criteria
- [ ] Smoke test compiles
- [ ] Links against libdosbox-staging.a without errors
- [ ] Runs and exits cleanly
- [ ] Reports "Hook works!"
- [ ] No undefined symbols

---

## PHASE 1 COMPLETION CHECKLIST

Before advancing to Phase 2, verify:

### Build System ✅
- [ ] BOXER_INTEGRATED=OFF builds standard DOSBox
- [ ] BOXER_INTEGRATED=ON builds static library
- [ ] All CMake changes guarded properly

### Infrastructure ✅
- [ ] boxer_hooks.h declares all 86 hooks
- [ ] IBoxerDelegate interface complete
- [ ] BOXER_HOOK macros defined and safe
- [ ] Global delegate pointer accessible

### Critical Hook ✅
- [ ] INT-059 integrated into normal_loop()
- [ ] Emergency abort mechanism in place
- [ ] Performance requirement understood

### Validation ✅
- [ ] Link test passes
- [ ] Smoke test runs
- [ ] No undefined symbols
- [ ] Both build modes work

### Documentation ✅
- [ ] All task reports filed
- [ ] Decisions logged
- [ ] PHASE_COMPLETE.md written

**When all boxes checked, Phase 1 is complete. Ready for Phase 2.**

---

## COMMON ISSUES AND SOLUTIONS

### Issue: CMake can't find boxer_hooks.h
**Solution**: Ensure include_directories() adds the right path:
```cmake
include_directories(${CMAKE_SOURCE_DIR}/include/boxer)
```

### Issue: Undefined symbol g_boxer_delegate
**Solution**: boxer_hooks.cpp must be added to library sources:
```cmake
list(APPEND DOSBOX_SOURCES src/boxer/boxer_hooks.cpp)
```

### Issue: normal_loop() not found
**Solution**: Search for alternate names:
```bash
grep -n "while.*true\|for.*;;\|loop" src/dosbox.cpp | head -20
```

### Issue: Smoke test won't link
**Solution**: Check library dependencies:
```bash
ldd libdosbox-staging.a  # Shows what it needs
```

---

## ESTIMATED TIME BREAKDOWN

- TASK 1-1: CMake Setup - 10-14 hours
- TASK 1-2: Hook Headers - 8-12 hours
- TASK 1-3: Stub Implementations - 6-10 hours
- TASK 1-4: CMake Source Integration - 4-6 hours
- TASK 1-5: INT-059 Integration - 8-12 hours
- TASK 1-6: Link Test - 4-6 hours

**Total**: 40-60 hours (plus validation and documentation overhead → 60-80 hours)

**Calendar time**: 1-2 weeks depending on blockers and human review cycles.
