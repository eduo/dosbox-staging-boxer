# Phase 2: Critical Lifecycle - Agent Tasks

**Phase Duration**: Weeks 3-4
**Total Estimated Hours**: 40-60 hours
**Goal**: Core emulation control working - start, stop, and emergency abort

**Prerequisites**: Phase 1 complete (DOSBox builds as library, hooks infrastructure in place)

---

## PHASE 2 OVERVIEW

By the end of Phase 2, you will have:
- Boxer can launch DOSBox emulation
- User can quit gracefully (window close works)
- Emergency abort mechanism functional (INT-059 implemented)
- No infinite loops or hangs
- Clean shutdown in all scenarios

**This phase produces the FIRST RUNTIME FUNCTIONALITY.**

---

## TASK 2-1: Emulation Loop Integration

### Context
- **Phase**: 2
- **Estimated Hours**: 12-16 hours
- **Criticality**: CRITICAL
- **Risk Level**: HIGH

### Objective
Complete the INT-059 implementation so Boxer can control the emulation loop and abort when needed.

### Prerequisites
- [ ] TASK 1-5 complete (INT-059 hook point added)
- [ ] Smoke test passes (hooks callable)
- [ ] Read unavoidable-modifications.md lines 857-1000

### Input Documents
1. `analysis/03-reintegration-analysis/unavoidable-modifications.md` lines 857-1000
   - Detailed INT-059 analysis
   - Performance requirements (<1μs)
   - Threading considerations

2. `src/dosbox-staging/src/dosbox.cpp`
   - Current normal_loop() with hook
   - Verify hook placement is correct

3. `src/boxer/Boxer/BXEmulator.mm`
   - Reference for how Boxer manages emulation lifecycle

### Deliverables
1. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement IBoxerDelegate for Objective-C++
   - Atomic cancellation flag
   - shouldContinueRunLoop() implementation
   
2. **Modified**: `src/boxer/Boxer/BXEmulator.mm`
   - Set g_boxer_delegate before emulation starts
   - Call cancel() on window close
   
3. **New file**: `validation/smoke-test/abort-test.mm`
   - Test that abort signal stops loop
   - Measure abort latency
   
4. **Documentation**: `progress/phase-2/tasks/TASK-2-1.md`

### Critical Implementation

```objc
// BoxerDelegate.mm
#include "boxer/boxer_hooks.h"
#import <atomic>

@interface BoxerDelegate : NSObject {
    std::atomic<bool> _cancelled;
}
@end

class BoxerDelegateImpl : public IBoxerDelegate {
private:
    BoxerDelegate* _owner;
    std::atomic<bool>& _cancelled;
    
public:
    BoxerDelegateImpl(BoxerDelegate* owner, std::atomic<bool>& cancelled)
        : _owner(owner), _cancelled(cancelled) {}
    
    bool shouldContinueRunLoop() override {
        // CRITICAL: Must be <1μs
        // Compiles to single atomic load instruction
        return !_cancelled.load(std::memory_order_relaxed);
    }
    
    // ... other hooks as stubs for now
};

@implementation BoxerDelegate

- (void)cancel {
    _cancelled.store(true, std::memory_order_relaxed);
}

- (void)startEmulation {
    _cancelled.store(false, std::memory_order_relaxed);
    // Set global delegate
    static BoxerDelegateImpl impl(self, _cancelled);
    g_boxer_delegate = &impl;
}

@end
```

### Performance Validation

```bash
# Must test INT-059 performance
# Create performance test
cat > validation/smoke-test/perf-test.cpp << 'EOF'
#include "boxer/boxer_hooks.h"
#include <chrono>
#include <iostream>

class PerfTestDelegate : public IBoxerDelegate {
    std::atomic<bool> cancelled{false};
public:
    bool shouldContinueRunLoop() override {
        return !cancelled.load(std::memory_order_relaxed);
    }
    // ... stub other methods
};

int main() {
    PerfTestDelegate delegate;
    g_boxer_delegate = &delegate;
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000000; i++) {
        BOXER_HOOK_BOOL(shouldContinueRunLoop);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double ns_per_call = ns / 10000000.0;
    
    std::cout << "shouldContinueRunLoop: " << ns_per_call << " ns/call\n";
    
    if (ns_per_call > 1000) {
        std::cerr << "FAIL: Exceeds 1μs requirement\n";
        return 1;
    }
    std::cout << "PASS: Within performance budget\n";
    return 0;
}
EOF
```

### Decision Points - STOP if:

1. **Performance exceeds budget**: If >1μs per call
   - Report: Actual timing, what's causing overhead
   - Options: Optimize implementation, reduce call frequency

2. **Thread safety issue**: If atomic operations aren't sufficient
   - Report: Race condition detected, symptoms
   - Options: Stronger memory ordering, mutex

3. **Delegate lifetime issue**: If delegate destroyed while loop running
   - Report: Crash or undefined behavior observed
   - Options: Different ownership model, ref counting

### Success Criteria
- [ ] shouldContinueRunLoop() called successfully from emulation loop
- [ ] Performance <1μs per call (10M iterations test)
- [ ] Atomic flag properly synchronized between threads
- [ ] No crashes or undefined behavior
- [ ] Abort signal stops emulation within 100ms

---

## TASK 2-2: Run Loop Lifecycle Hooks

### Context
- **Phase**: 2
- **Estimated Hours**: 8-12 hours
- **Criticality**: MAJOR
- **Risk Level**: MEDIUM

### Objective
Add runLoopWillStart() and runLoopDidFinish() hooks so Boxer can initialize and cleanup around emulation.

### Prerequisites
- [ ] TASK 2-1 complete (basic abort works)
- [ ] Read unavoidable-modifications.md lines 792-854

### Input Documents
1. `analysis/03-reintegration-analysis/unavoidable-modifications.md` lines 792-854
   - INT-077, INT-078 details
   - Hook placement strategy

2. `src/dosbox-staging/src/dosbox.cpp`
   - Main emulation entry point
   - Exception handling structure

### Deliverables
1. **Modified**: `src/dosbox-staging/src/dosbox.cpp`
   - Add runLoopWillStart hook before normal_loop()
   - Add runLoopDidFinish hook after normal_loop()
   - ~6 lines added
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement runLoopWillStart() - initialize Metal, audio
   - Implement runLoopDidFinish() - cleanup resources
   
3. **Documentation**: `progress/phase-2/tasks/TASK-2-2.md`

### Implementation Pattern

```cpp
// In dosbox.cpp
static void DOSBOX_RunMachine() {
    #ifdef BOXER_INTEGRATED
    BOXER_HOOK_VOID(runLoopWillStart);
    #endif

    try {
        normal_loop();
    } catch (const EExit& e) {
        // Handle exit
    } catch (...) {
        // Handle other exceptions
    }

    #ifdef BOXER_INTEGRATED
    BOXER_HOOK_VOID(runLoopDidFinish);
    #endif
}
```

### Success Criteria
- [ ] runLoopWillStart() called exactly once per emulation session
- [ ] runLoopDidFinish() called even if emulation aborts
- [ ] Exception safety - hooks called in all exit paths
- [ ] Boxer can initialize resources before emulation
- [ ] Boxer can cleanup resources after emulation

---

## TASK 2-3: Window Close Integration

### Context
- **Phase**: 2
- **Estimated Hours**: 8-10 hours
- **Criticality**: CORE
- **Risk Level**: MEDIUM

### Objective
Connect Boxer's window close button to the emulation abort mechanism.

### Prerequisites
- [ ] TASK 2-1 complete (abort mechanism works)
- [ ] TASK 2-2 complete (lifecycle hooks in place)

### Input Documents
1. `src/boxer/Boxer/BXEmulator.mm`
   - Current window management code
   - How close is currently handled

2. `src/boxer/Boxer/BXSession.m`
   - Session lifecycle management
   - Window controller relationship

### Deliverables
1. **Modified**: `src/boxer/Boxer/BXEmulator.mm`
   - Connect windowWillClose: to cancel()
   - Handle graceful shutdown sequence
   
2. **Test**: `validation/smoke-test/window-close-test.mm`
   - Simulate window close
   - Verify emulation stops
   - Check resource cleanup
   
3. **Documentation**: `progress/phase-2/tasks/TASK-2-3.md`

### Integration Pattern

```objc
// In BXEmulator.mm or BXSession.m
- (void)windowWillClose:(NSNotification *)notification {
    // Signal emulation to stop
    [self.delegate cancel];
    
    // Wait for emulation thread to finish (with timeout)
    [self waitForEmulationToStop:5.0]; // 5 second timeout
    
    // Cleanup
    [self cleanupResources];
}

- (void)waitForEmulationToStop:(NSTimeInterval)timeout {
    NSDate *deadline = [NSDate dateWithTimeIntervalSinceNow:timeout];
    while ([self isEmulationRunning] && [[NSDate date] compare:deadline] == NSOrderedAscending) {
        [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
    }
    
    if ([self isEmulationRunning]) {
        NSLog(@"WARNING: Emulation did not stop within timeout");
        // Force stop if needed
    }
}
```

### Success Criteria
- [ ] Clicking window close stops emulation
- [ ] No hangs (completes within 5 seconds)
- [ ] Resources properly cleaned up
- [ ] No crashes on quit
- [ ] Application can quit cleanly

---

## TASK 2-4: Emergency Abort Testing

### Context
- **Phase**: 2
- **Estimated Hours**: 6-10 hours
- **Criticality**: MAJOR
- **Risk Level**: LOW

### Objective
Comprehensive testing of abort mechanism under various conditions.

### Prerequisites
- [ ] TASK 2-3 complete (window close integrated)
- [ ] All lifecycle hooks working

### Deliverables
1. **Test suite**: `validation/contracts/test_lifecycle.cpp`
   - Test normal startup and shutdown
   - Test abort during initialization
   - Test abort during active emulation
   - Test abort during I/O operation
   - Test rapid start/stop cycles
   
2. **Performance benchmark**: `validation/contracts/benchmark_abort.cpp`
   - Measure abort latency
   - Measure overhead impact
   - Stress test with many cycles
   
3. **Documentation**: `progress/phase-2/tasks/TASK-2-4.md`

### Test Cases

```cpp
TEST(Lifecycle, NormalShutdown) {
    // Start emulation, run briefly, stop normally
    // Expected: Clean exit, resources freed
}

TEST(Lifecycle, AbortDuringInit) {
    // Signal abort before loop starts
    // Expected: Never enters loop, clean exit
}

TEST(Lifecycle, AbortDuringExecution) {
    // Signal abort while emulation running
    // Expected: Stops within 100ms
}

TEST(Lifecycle, RapidCycles) {
    // Start/stop 100 times rapidly
    // Expected: No leaks, no crashes
}

TEST(Lifecycle, AbortLatency) {
    // Measure time from cancel() to loop exit
    // Expected: <100ms
}

TEST(Lifecycle, MemoryLeakCheck) {
    // Check for memory leaks after cycles
    // Expected: No leaks
}
```

### Success Criteria
- [ ] All test cases pass
- [ ] Abort latency <100ms in all scenarios
- [ ] No memory leaks detected
- [ ] No crashes or hangs
- [ ] Performance overhead <1%

---

## TASK 2-5: Error Handling and Edge Cases

### Context
- **Phase**: 2
- **Estimated Hours**: 6-8 hours
- **Criticality**: MAJOR
- **Risk Level**: MEDIUM

### Objective
Handle error conditions gracefully - what if DOSBox throws exception, what if init fails, etc.

### Prerequisites
- [ ] TASK 2-4 complete (basic testing done)

### Deliverables
1. **Modified**: `src/dosbox-staging/src/dosbox.cpp`
   - Add error reporting hook
   - Handle exceptions properly
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement error callback
   - Report errors to Boxer UI
   
3. **Test**: Error condition test suite
   
4. **Documentation**: `progress/phase-2/tasks/TASK-2-5.md`

### Error Hook Pattern

```cpp
// In dosbox.cpp
static void DOSBOX_RunMachine() {
    #ifdef BOXER_INTEGRATED
    BOXER_HOOK_VOID(runLoopWillStart);
    #endif

    try {
        normal_loop();
    } catch (const EExit& e) {
        #ifdef BOXER_INTEGRATED
        BOXER_HOOK_VOID(emulationDidExitNormally, e.code);
        #endif
    } catch (const std::exception& e) {
        #ifdef BOXER_INTEGRATED
        BOXER_HOOK_VOID(emulationDidEncounterError, e.what());
        #endif
    } catch (...) {
        #ifdef BOXER_INTEGRATED
        BOXER_HOOK_VOID(emulationDidEncounterError, "Unknown error");
        #endif
    }

    #ifdef BOXER_INTEGRATED
    BOXER_HOOK_VOID(runLoopDidFinish);
    #endif
}
```

### Success Criteria
- [ ] DOSBox exceptions reported to Boxer
- [ ] UI can display error messages
- [ ] No silent failures
- [ ] Cleanup happens even on error
- [ ] User informed of problems

---

## PHASE 2 COMPLETION CHECKLIST

Before advancing to Phase 3, verify:

### Core Functionality ✅
- [ ] Emulation can start
- [ ] Emulation can stop (gracefully)
- [ ] Emergency abort works (<100ms)
- [ ] Window close quits cleanly

### Performance ✅
- [ ] shouldContinueRunLoop <1μs overhead
- [ ] Overall overhead <1% of emulation time
- [ ] No performance regression

### Stability ✅
- [ ] No crashes in any scenario
- [ ] No memory leaks
- [ ] No infinite loops
- [ ] Rapid start/stop works

### Error Handling ✅
- [ ] Exceptions properly caught
- [ ] Errors reported to UI
- [ ] Cleanup always happens

### Testing ✅
- [ ] All lifecycle tests pass
- [ ] Performance benchmarks meet requirements
- [ ] Edge cases handled

### Documentation ✅
- [ ] All task reports filed
- [ ] Decisions logged
- [ ] PHASE_COMPLETE.md written

**When all boxes checked, Phase 2 is complete. Ready for Phase 3 (Rendering).**

---

## COMMON ISSUES AND SOLUTIONS

### Issue: Abort doesn't stop emulation
**Cause**: Hook not being called in loop
**Solution**: Verify normal_loop() structure, ensure hook is inside while()

### Issue: Performance exceeds 1μs
**Cause**: Wrong memory ordering or virtual call overhead
**Solution**: Use memory_order_relaxed, ensure inlined

### Issue: Delegate destroyed while loop running
**Cause**: Lifetime management issue
**Solution**: Make delegate outlive emulation, use shared ownership

### Issue: Window close hangs
**Cause**: Emulation not responding to abort
**Solution**: Add timeout, force-quit after deadline

### Issue: Resources not cleaned up
**Cause**: runLoopDidFinish not called on error path
**Solution**: Ensure finally-like behavior (try/catch/finally pattern)

---

## ESTIMATED TIME BREAKDOWN

- TASK 2-1: Emulation Loop Integration - 12-16 hours
- TASK 2-2: Run Loop Lifecycle Hooks - 8-12 hours
- TASK 2-3: Window Close Integration - 8-10 hours
- TASK 2-4: Emergency Abort Testing - 6-10 hours
- TASK 2-5: Error Handling - 6-8 hours

**Total**: 40-60 hours (matches consolidated-strategy.md)

**Calendar time**: 1-2 weeks depending on testing depth.