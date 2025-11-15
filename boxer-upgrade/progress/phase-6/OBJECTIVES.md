# Phase 6: Parport Migration - Objectives

**Phase**: 6
**Duration**: Weeks 11-12
**Estimated Hours**: 27-33
**Status**: NOT STARTED

---

## Primary Goal
Printer functionality restored with ~4,000 lines migrated.

---

## Objectives

### 1. Parport Code Migration
Migrate parallel port emulation from legacy DOSBox to target.

**Success Criteria**:
- All parport source files migrated
- Compiles with modern DOSBox APIs
- IO ports functional

### 2. Printer Redirection
Restore printer redirection and capture functionality.

**Success Criteria**:
- Print data captured
- Print preview works
- PDF generation functional

### 3. LPT DAC Conflict Resolution
Resolve parallel port usage conflict per DEC-002.

**Success Criteria**:
- Conflict handled per decision
- No runtime errors
- Both features accessible (if applicable)

---

## Deliverables

1. **Code**:
   - parport.cpp/h migrated
   - printer_redir.cpp/h migrated
   - LPT DAC resolution

2. **Tests**:
   - Printer functionality tests
   - DOS program printing tests

3. **Documentation**:
   - Migration notes
   - API adaptations

---

## Dependencies

**Prerequisites**:
- Phase 5 complete
- DEC-002 resolved (LPT DAC conflict)

**Blocking Decisions**:
- DEC-002: Runtime detection vs. compile-time vs. single feature

---

## Phase Exit Criteria

- [ ] Parport code migrated
- [ ] Printer emulation works
- [ ] Print preview functional
- [ ] PDF generation works
- [ ] LPT DAC conflict resolved
- [ ] Human review approved

**Ready for Phase 7 when all criteria met.**
