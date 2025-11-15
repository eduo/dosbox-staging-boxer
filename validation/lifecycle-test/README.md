# Lifecycle Hook Test Suite

Comprehensive test suite for validating Boxer-DOSBox lifecycle integration hooks.

## Purpose

Tests the three critical lifecycle hooks that enable Boxer to control DOSBox emulation:

1. **INT-059: `runLoopShouldContinue`** - Emergency abort mechanism (called ~10,000/sec)
2. **INT-077: `runLoopWillStartWithContextInfo`** - Initialize resources before emulation
3. **INT-078: `runLoopDidFinishWithContextInfo`** - Clean up resources after emulation

## Test Cases

### TEST 1: Normal Lifecycle
- Verifies hooks called in correct order
- Runs 100 iterations then stops normally
- Validates: WillStart → ShouldContinue (×100) → DidFinish

### TEST 2: Abort During Execution
- Starts emulation in thread
- Signals abort after 50ms
- Measures abort latency
- Validates cleanup happens after abort
- **Requirement**: Abort latency <100ms

### TEST 3: Immediate Abort
- Signals abort before emulation starts
- Verifies emulation stops on first iteration
- Validates cleanup still happens

### TEST 4: Rapid Cycles
- Runs 100 rapid start/stop cycles
- Tests for memory leaks, crashes, hangs
- Validates stability under stress

### TEST 5: Hook Call Order
- Explicitly validates hook call sequence
- Ensures WillStart before ShouldContinue
- Ensures DidFinish after loop ends

## Building

```bash
cd validation/lifecycle-test
mkdir build && cd build
cmake ..
cmake --build .
```

## Running

```bash
./lifecycle-test
```

## Expected Output

```
========================================
Boxer Lifecycle Hook Test Suite
========================================

Testing INT-057, INT-058, INT-059 integration

[TEST 1] Normal Lifecycle (start → run briefly → stop)
  [HOOK] runLoopWillStart called (context=nullptr)
  [HOOK] runLoopDidFinish called (context=nullptr) (iterations=100)
  ✓ WillStart called
  ✓ DidFinish called
  ✓ Ran exactly 100 iterations
  ✅ TEST PASSED

[TEST 2] Abort During Execution
  [HOOK] runLoopWillStart called (context=nullptr)
  [ACTION] Signaling abort...
  [HOOK] runLoopDidFinish called (context=nullptr) (iterations=XXX)
  ✓ WillStart called
  ✓ DidFinish called (cleanup happened)
  ✓ Ran XXX iterations before abort
  Abort latency: XXXX μs - ✓ Within 100ms requirement
  ✅ TEST PASSED

... (additional tests)

========================================
Test Summary:
  Passed: 5/5
  Failed: 0/5
========================================

✅ ALL TESTS PASSED - Lifecycle hooks working correctly!
```

## Success Criteria

All 5 tests must pass:
- [x] Normal lifecycle works
- [x] Abort during execution works and is fast (<100ms)
- [x] Immediate abort works
- [x] No crashes/leaks in rapid cycles
- [x] Hook call order is correct

## Integration with DOSBox

This test uses a **simulated** emulation loop that mirrors the structure of DOSBox's actual emulation loop:

```cpp
void simulateEmulationLoop() {
    BOXER_HOOK_VOID(runLoopWillStartWithContextInfo, nullptr);  // INT-077

    while (true) {
        if (!BOXER_HOOK_BOOL(runLoopShouldContinue)) {          // INT-059
            break;
        }
        // ... emulation work
    }

    BOXER_HOOK_VOID(runLoopDidFinishWithContextInfo, nullptr);  // INT-078
}
```

This matches the actual DOSBox integration:
- `src/dosbox.cpp`: `DOSBOX_RunMachine()` calls `runLoopWillStart`
- `src/dosbox.cpp`: `normal_loop()` checks `runLoopShouldContinue`
- `src/dosbox.cpp`: `DOSBOX_RunMachine()` calls `runLoopDidFinish`

## Dependencies

- C++17 compiler
- CMake 3.16+
- pthread (for multi-threaded abort test)
- Boxer hook headers (`include/boxer/boxer_hooks.h`)

## Notes

- Tests are standalone and don't require full DOSBox library
- All 83 IBoxerDelegate methods have stub implementations
- Test focuses on the 3 critical lifecycle hooks
- Performance requirement: INT-059 must complete in <1μs (tested separately in perf-test)
- Abort latency requirement: <100ms from cancel() to loop exit

## Related Tests

- **smoke-test**: Basic hook invocation test
- **perf-test**: INT-059 performance benchmark (10M iterations)

## Phase 2 Deliverable

This test suite is a key deliverable for **Phase 2: Critical Lifecycle**.

**Task**: TASK 2-2
**Integration Points**: INT-057, INT-058, INT-059
**Success Gate**: All 5 tests must pass before advancing to Phase 3
