# Phase 6: Parport Migration - Agent Tasks

**Phase Duration**: Weeks 11-12
**Total Estimated Hours**: 27-33 hours
**Goal**: Printer functionality restored

**Prerequisites**: Phase 5 complete (file I/O security working)

**⚠️ STRATEGIC DECISION REQUIRED**: DEC-002 (LPT DAC conflict) must be resolved before Phase 6

---

## PHASE 6 OVERVIEW

By the end of Phase 6, you will have:
- Printer emulation functional (~4,000 lines migrated)
- LPT1-3 ports working
- Print preview operational
- PDF generation works
- LPT DAC conflict resolved

**This phase migrates code from DOSBox Daum that was never in official DOSBox Staging.**

---

## CRITICAL INTEGRATION POINTS

From integration-overview.md:
- INT-075: printerWriteData
- INT-076: printerReadStatus
- INT-077: printerWriteControl
- INT-078: printerReadControl
- INT-079: printerInitialize
- INT-080: printerFinalize

---

## TASK 6-1: Parport Source Analysis

### Context
- **Phase**: 6
- **Estimated Hours**: 4-6 hours
- **Criticality**: CORE
- **Risk Level**: MEDIUM

### Objective
Analyze the parport code in legacy DOSBox and plan migration.

### Prerequisites
- [ ] Phase 5 complete
- [ ] DEC-002 resolved (LPT DAC conflict decision)

### Input Documents
1. `src/dosbox-staging-legacy/src/hardware/parport/`
   - All parport source files
   - Approximately 4,000 lines

2. `analysis/04-special-cases/parport-migration.md`
   - Complete migration strategy (1,042 lines)

3. `src/dosbox-staging/src/hardware/`
   - Verify parport absent in target

### Deliverables
1. **Inventory**: List all parport source files and dependencies
   
2. **API mapping**: What parport functions need what DOSBox APIs
   
3. **Conflict analysis**: Identify incompatibilities with modern DOSBox
   
4. **Documentation**: `progress/phase-6/tasks/TASK-6-1.md`

### Files to Migrate
```
src/hardware/parport/
├── parport.cpp         # Core parallel port emulation
├── parport.h           # Header
├── printer_redir.cpp   # Printer redirection logic
├── printer_redir.h     # Printer header
└── [other files]       # List all
```

### Success Criteria
- [ ] Complete file inventory
- [ ] Dependencies mapped
- [ ] API compatibility assessed
- [ ] Migration plan validated

---

## TASK 6-2: Parport Directory Setup

### Context
- **Phase**: 6
- **Estimated Hours**: 2-3 hours
- **Criticality**: MINOR
- **Risk Level**: LOW

### Objective
Create parport directory structure in target DOSBox.

### Prerequisites
- [ ] TASK 6-1 complete (analysis done)

### Deliverables
1. **New directory**: `src/dosbox-staging/src/hardware/parport/`
   
2. **CMake integration**: Add parport to build system
   
3. **Documentation**: `progress/phase-6/tasks/TASK-6-2.md`

### CMake Addition

```cmake
if(BOXER_INTEGRATED)
    list(APPEND HARDWARE_SOURCES
        hardware/parport/parport.cpp
        hardware/parport/printer_redir.cpp
    )
endif()
```

### Success Criteria
- [ ] Directory created
- [ ] Added to CMake (when BOXER_INTEGRATED=ON)
- [ ] Structure matches legacy

---

## TASK 6-3: Core Parport Migration

### Context
- **Phase**: 6
- **Estimated Hours**: 10-12 hours
- **Criticality**: CORE
- **Risk Level**: HIGH

### Objective
Migrate core parallel port emulation code.

### Prerequisites
- [ ] TASK 6-2 complete (directory setup)

### Deliverables
1. **Migrated**: `src/dosbox-staging/src/hardware/parport/parport.cpp`
   - Updated to modern DOSBox APIs
   - Guarded by BOXER_INTEGRATED
   
2. **Migrated**: `src/dosbox-staging/src/hardware/parport/parport.h`
   
3. **Test**: Basic parport functionality
   
4. **Documentation**: `progress/phase-6/tasks/TASK-6-3.md`

### Migration Challenges
1. **API changes**: IO port registration may differ
2. **Class hierarchy**: May need to adapt to modern DOSBox device model
3. **Initialization**: Device setup sequence may have changed
4. **Configuration**: Config file format may differ

### Implementation Pattern

```cpp
// In parport.cpp
#ifdef BOXER_INTEGRATED

#include "parport.h"
#include "boxer/boxer_hooks.h"

class CParallel : public IO_Device {
public:
    CParallel(/* ... */) {
        // Register IO ports
        // Setup callbacks
    }
    
    void Write_LPT(uint16_t port, uint8_t val) {
        BOXER_HOOK_VOID(printerWriteData, port, val);
    }
    
    uint8_t Read_LPT_Status(uint16_t port) {
        return BOXER_HOOK_VALUE(printerReadStatus, 0xFF, port);
    }
};

#endif // BOXER_INTEGRATED
```

### Decision Points - STOP if:

1. **IO port registration changed**: Modern DOSBox uses different API
   - Report: Old vs. new API, adaptation needed

2. **Device initialization sequence**: Different startup order
   - Report: What's expected, what's available

### Success Criteria
- [ ] Parport compiles with modern DOSBox
- [ ] IO ports registered correctly
- [ ] Basic port operations work
- [ ] No build errors

---

## TASK 6-4: Printer Redirection Migration

### Context
- **Phase**: 6
- **Estimated Hours**: 8-10 hours
- **Criticality**: MAJOR
- **Risk Level**: MEDIUM

### Objective
Migrate printer redirection code (the actual printing logic).

### Prerequisites
- [ ] TASK 6-3 complete (core parport works)

### Deliverables
1. **Migrated**: `src/dosbox-staging/src/hardware/parport/printer_redir.cpp`
   
2. **Migrated**: `src/dosbox-staging/src/hardware/parport/printer_redir.h`
   
3. **Modified**: `src/boxer/Boxer/BXEmulatedPrinter.m`
   - Connect to parport hooks
   - Handle print data
   
4. **Test**: Print output test
   
5. **Documentation**: `progress/phase-6/tasks/TASK-6-4.md`

### Boxer Printer Features
- Capture printed data to buffer
- Convert to PDF
- Print preview
- Physical printer output via macOS

### Success Criteria
- [ ] Printer data captured
- [ ] Format conversion works
- [ ] Print preview functional
- [ ] PDF generation works

---

## TASK 6-5: LPT DAC Conflict Resolution

### Context
- **Phase**: 6
- **Estimated Hours**: 4-6 hours
- **Criticality**: MAJOR
- **Risk Level**: MEDIUM

### Objective
Implement the decided resolution for LPT DAC vs. printer conflict.

### Prerequisites
- [ ] TASK 6-4 complete (printer works)
- [ ] DEC-002 resolved (decision made by human)

### Deliverables (vary by decision)

**If Runtime Detection (Option A)**:
1. Detection logic to identify what's connected
2. Configuration UI for user override
3. Switching logic at runtime

**If Compile-time Flag (Option B)**:
1. Build-time flag
2. Documentation of two build variants

**If Printer-only (Option C)**:
1. Remove LPT DAC code paths
2. Document removal

### Documentation
`progress/phase-6/tasks/TASK-6-5.md`

### Success Criteria
- [ ] Conflict resolved per decision
- [ ] No crashes when both features present
- [ ] User can access both (or chosen) features
- [ ] Behavior documented

---

## TASK 6-6: Printer Integration Testing

### Context
- **Phase**: 6
- **Estimated Hours**: 4-6 hours
- **Criticality**: MAJOR
- **Risk Level**: LOW

### Objective
Comprehensive testing of printer functionality.

### Prerequisites
- [ ] TASK 6-5 complete (conflict resolved)

### Deliverables
1. **Test suite**: Printer emulation tests
   - Print text document
   - Print graphics
   - Multiple pages
   - Printer status queries
   
2. **Integration test**: Real DOS programs printing
   
3. **Documentation**: `progress/phase-6/tasks/TASK-6-6.md`

### Test Programs
- WordPerfect print test
- Banner.com
- DIR > LPT1
- Graphics print (GRAPHICS.COM)

### Success Criteria
- [ ] Text printing works
- [ ] Graphics printing works
- [ ] Multi-page documents work
- [ ] Print preview shows output
- [ ] PDF generation correct

---

## PHASE 6 COMPLETION CHECKLIST

### Core Migration ✅
- [ ] Parport code migrated (~4,000 lines)
- [ ] Compiles with modern DOSBox
- [ ] IO ports functional
- [ ] No API incompatibilities

### Printer Features ✅
- [ ] Print data captured
- [ ] Print preview works
- [ ] PDF generation works
- [ ] Multiple LPT ports work

### Conflict Resolution ✅
- [ ] LPT DAC conflict handled
- [ ] Per DEC-002 decision
- [ ] No runtime errors

### Integration ✅
- [ ] Boxer hooks connected
- [ ] UI updates correctly
- [ ] Full testing passed

**When all boxes checked, Phase 6 is complete. Ready for Phase 7 (Input/Audio).**

---

## ESTIMATED TIME BREAKDOWN

- TASK 6-1: Parport Source Analysis - 4-6 hours
- TASK 6-2: Directory Setup - 2-3 hours
- TASK 6-3: Core Migration - 10-12 hours
- TASK 6-4: Printer Redirection - 8-10 hours
- TASK 6-5: LPT DAC Resolution - 4-6 hours
- TASK 6-6: Integration Testing - 4-6 hours

**Total**: 32-43 hours (~27-33 planned, slightly over)

**Calendar time**: 1-1.5 weeks