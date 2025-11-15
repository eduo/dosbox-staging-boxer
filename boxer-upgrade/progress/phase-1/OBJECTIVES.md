# Phase 1: Foundation - Objectives

**Duration**: Weeks 1-2
**Estimated Hours**: 60-80 hours
**Status**: IN PROGRESS
**Started**: 2025-11-15

---

## Goal

Establish build system and core infrastructure for Boxer DOSBox Staging integration.

**This phase produces NO runtime functionality.** Everything is scaffolding and infrastructure.

---

## Success Criteria

By the end of Phase 1, the following must be achieved:

### Build System ✅
- [ ] DOSBox Staging builds as static library with `BOXER_INTEGRATED=ON`
- [ ] Standard DOSBox build works unchanged with `BOXER_INTEGRATED=OFF`
- [ ] All CMake changes properly guarded with conditionals
- [ ] CMake 3.16+ syntax compliance maintained

### Hook Infrastructure ✅
- [ ] `IBoxerDelegate` interface declared with all 86 integration points
- [ ] `boxer_hooks.h` compiles standalone
- [ ] BOXER_HOOK macros defined with safe defaults
- [ ] Global delegate pointer accessible from DOSBox code
- [ ] All hook methods documented with performance requirements

### Critical Hook Integration ✅
- [ ] INT-059 (shouldContinueRunLoop) integrated into `normal_loop()`
- [ ] Emergency abort mechanism functional
- [ ] Performance requirement understood (<1μs, ~10,000 calls/sec)

### Validation ✅
- [ ] Link test passes (can link against libdosbox-staging.a)
- [ ] Smoke test runs (stub delegate works)
- [ ] No undefined symbols
- [ ] Both BOXER_INTEGRATED=ON and OFF build modes functional

### Documentation ✅
- [ ] All 6 task reports filed in `progress/phase-1/tasks/`
- [ ] All decisions logged in `DECISION_LOG.md`
- [ ] `PHASE_COMPLETE.md` written

---

## Deliverables

1. **Modified CMakeLists.txt** - BOXER_INTEGRATED option (~43 lines)
2. **boxer_hooks.h** - IBoxerDelegate interface (~170 lines)
3. **boxer_types.h** - Shared type definitions
4. **boxer_hooks.cpp** - Stub implementations (minimal)
5. **boxer_config.h** - Configuration header
6. **BoxerIntegration.cmake** - Helper module (optional)
7. **Link test harness** - Validation smoke test
8. **First integration point** - INT-059 in dosbox.cpp

---

## Tasks Breakdown

### TASK 1-1: Directory and CMake Setup
- **Hours**: 10-14
- **Criticality**: CORE
- **Status**: PENDING

### TASK 1-2: Hook Infrastructure Headers
- **Hours**: 8-12
- **Criticality**: CORE
- **Status**: PENDING

### TASK 1-3: Stub Implementations
- **Hours**: 6-10
- **Criticality**: MEDIUM
- **Status**: PENDING

### TASK 1-4: CMake Source Integration
- **Hours**: 4-6
- **Criticality**: CORE
- **Status**: PENDING

### TASK 1-5: First Integration Point (INT-059)
- **Hours**: 8-12
- **Criticality**: CRITICAL
- **Status**: PENDING

### TASK 1-6: Link Test Harness
- **Hours**: 4-6
- **Criticality**: MAJOR
- **Status**: PENDING

**Total**: 40-60 hours core work + 20 hours validation/documentation = 60-80 hours

---

## Dependencies from Previous Work

### Confirmed Pre-Validated Conditions
- ✅ `normal_loop()` exists in target DOSBox
- ✅ MidiDevice interface is stable
- ✅ Parport completely absent from target (must add in Phase 6)

### Required Repository Structure
- ✅ boxer-upgrade/ directory created
- ✅ All analysis documents available
- ✅ Phase 1 task instructions ready

---

## Known Risks

### Risk 1: CMake Version Conflicts
- **Likelihood**: LOW
- **Impact**: MEDIUM
- **Mitigation**: Validate CMake syntax against target DOSBox version first

### Risk 2: Source File Structure Changes
- **Likelihood**: MEDIUM
- **Impact**: MEDIUM
- **Mitigation**: Check DOSBOX_SOURCES variable structure before modifying

### Risk 3: normal_loop() Location Changed
- **Likelihood**: LOW
- **Impact**: HIGH
- **Mitigation**: Search for loop structure variations if not found

---

## Validation Gates

### Gate 0: Pre-Phase ✅
- [x] Phase objectives documented (this file)
- [x] Success criteria defined
- [x] Estimated hours reviewed
- [x] No dependencies from previous phases (Phase 1 is first)
- [x] Required analysis documents identified

### Gate 1: Static Analysis
Must pass after each file modification

### Gate 2: Consistency Check
Must pass after completing all tasks

### Gate 3: Human Review
Required before advancing to Phase 2

---

## Next Phase Preview

**Phase 2: Critical Lifecycle** will implement:
- Remaining lifecycle callbacks (INT-057, INT-058)
- Emergency abort mechanism testing
- Full emulation start/stop/quit functionality

**Phase 2 depends on**: All Phase 1 success criteria met
