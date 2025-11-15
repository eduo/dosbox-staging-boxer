# Phase 2: Critical Lifecycle - Objectives

**Phase**: 2
**Duration**: Weeks 3-4
**Estimated Hours**: 40-60 (estimated), 15-25 (expected based on Phase 1 efficiency)
**Status**: IN PROGRESS
**Started**: 2025-11-15

---

## Primary Goal
Achieve core emulation control: start, stop, and emergency abort functionality.

---

## Phase 2 Task Breakdown

**IMPORTANT**: Phase 1 already implemented INT-059 (runLoopShouldContinue) in dosbox.cpp.
Phase 2 focuses on:
1. Adding the remaining lifecycle hooks (INT-077, INT-078)
2. Creating comprehensive tests
3. Performance validation

### TASK 2-1: Add Remaining Lifecycle Hooks (6-8 hours)
Add INT-077 and INT-078 to dosbox.cpp for emulation start/finish notifications.

**Work Items**:
- Add runLoopWillStartWithContextInfo before normal_loop()
- Add runLoopDidFinishWithContextInfo after normal_loop()
- Ensure exception-safe placement
- Verify all exit paths covered

**Success Criteria**:
- [ ] Hooks called exactly once per emulation session
- [ ] Exception-safe (called even on error exit)
- [ ] All changes guarded by BOXER_INTEGRATED
- [ ] Standard build unaffected

### TASK 2-2: Create Lifecycle Test Suite (10-12 hours)
Comprehensive C++ tests to validate all lifecycle scenarios.

**Test Cases**:
1. Normal lifecycle (start → run → stop)
2. Abort before start
3. Abort during execution
4. Abort after finish
5. Rapid start/stop cycles (100+)
6. Latency measurement

**Success Criteria**:
- [ ] All test cases pass
- [ ] Abort latency <100ms
- [ ] No memory leaks
- [ ] No crashes or hangs

### TASK 2-3: Performance Validation (6-8 hours)
Validate INT-059 meets <1μs performance requirement.

**Benchmarks**:
- 10 million iterations test
- Memory ordering validation
- Overhead measurement

**Success Criteria**:
- [ ] Average call time <1μs
- [ ] Total overhead <1%
- [ ] Optimal atomic memory ordering
- [ ] Results documented

### TASK 2-4: Update Smoke Test (2-4 hours)
Extend Phase 1 smoke test to validate lifecycle integration.

**Success Criteria**:
- [ ] All 3 lifecycle hooks tested
- [ ] Hook call order validated
- [ ] Context info passed correctly
- [ ] No undefined behavior

---

## Deliverables

1. **Code**:
   - BoxerDelegate implementation in Objective-C++
   - Atomic cancellation flag
   - Lifecycle hooks in dosbox.cpp
   - Error reporting hooks

2. **Tests**:
   - Abort latency measurement
   - Performance benchmark (10M iterations)
   - Rapid start/stop stress test
   - Memory leak detection

3. **Documentation**:
   - Task reports
   - Performance results
   - Integration patterns

---

## Dependencies

**Prerequisites**:
- Phase 1 complete
- Library builds successfully
- Hooks infrastructure in place

**Blocking Decisions**:
- None

---

## Risk Assessment

**High Risk**:
- Performance exceeds 1μs budget (Mitigation: Profile and optimize)

**Medium Risk**:
- Thread safety issues (Mitigation: Proper atomic operations)

**Low Risk**:
- Lifetime management (Mitigation: Clear ownership model)

---

## Phase Exit Criteria

- [ ] Emulation can start
- [ ] Emulation can stop gracefully
- [ ] Emergency abort works (<100ms)
- [ ] Window close quits cleanly
- [ ] Performance <1μs overhead
- [ ] No memory leaks
- [ ] Human review approved

**Ready for Phase 3 when all criteria met.**
