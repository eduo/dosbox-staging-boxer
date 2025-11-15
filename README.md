# Boxer DOSBox Staging Upgrade Project

Coordinated multi-agent upgrade of Boxer from legacy DOSBox Staging (9000+ commits behind) to modern DOSBox Staging.

## Quick Start

### 1. Create Project Structure
```bash
mkdir -p boxer-upgrade
cd boxer-upgrade

# Create directory structure
mkdir -p instructions/phases
mkdir -p analysis/{00-orchestrator,01-current-integration,03-reintegration-analysis,04-special-cases}
mkdir -p progress/phase-{1..8}/{tasks,validation}
mkdir -p validation/{smoke-test,contracts}
mkdir -p src
mkdir -p build/{dosbox-staging-normal,dosbox-staging-boxer,boxer}
```

### 2. Clone Source Repositories
```bash
cd src

# Boxer (your fork)
git clone https://github.com/eduo/Boxer.git boxer
cd boxer
git checkout dosbox-boxer-upgrade-boxerside
cd ..

# Target DOSBox Staging (modern)
git clone https://github.com/eduo/dosbox-staging.git dosbox-staging
cd dosbox-staging
git checkout dosbox-boxer-upgrade-dosboxside
cd ..

# Legacy DOSBox Staging (reference only)
git clone https://github.com/eduo/dosbox-staging-boxer.git dosbox-staging-legacy
cd dosbox-staging-legacy
git checkout main
cd ..
```

### 3. Copy Analysis Documents
```bash
# Copy the analysis files from Claude Code Web session
# These should go in analysis/ directory
```

### 4. Copy Instruction Files
Copy the generated instruction files:
- `MASTER_ORCHESTRATOR.md` → `instructions/`
- `phase-1-foundation.md` → `instructions/phases/`
- `DECISION_LOG.md` → project root
- `VALIDATION_GATES.md` → `instructions/`

### 5. Initialize Progress Tracking
```bash
# Create initial PROGRESS.md
cat > PROGRESS.md << 'EOF'
# Boxer DOSBox Upgrade - Progress Tracker

## Current Status
- **Phase**: 1 (Foundation)
- **Status**: NOT STARTED
- **Last Updated**: [DATE]

## Phase Progress
- Phase 1 (Foundation): NOT STARTED
- Phase 2 (Lifecycle): NOT STARTED
- Phase 3 (Rendering): NOT STARTED
- Phase 4 (Shell): NOT STARTED
- Phase 5 (File I/O): NOT STARTED
- Phase 6 (Parport): NOT STARTED
- Phase 7 (Input/Audio): NOT STARTED
- Phase 8 (Testing): NOT STARTED

## Hours Spent
- Estimated Total: 525-737 hours
- Actual Total: 0 hours
- Current Phase: 0 hours

## Active Blockers
None yet.

## Recent Activity
- [DATE]: Project initialized
EOF
```

### 6. Validate Setup
```bash
# Check all repos present
ls -la src/
# Should show: boxer, dosbox-staging, dosbox-staging-legacy

# Check DOSBox can configure
cd build/dosbox-staging-normal
cmake ../../src/dosbox-staging/
# Should succeed (no Boxer modifications yet)
```

---

## Project Overview

### What This Project Does
Upgrades Boxer's embedded DOSBox from a fork that's 9000+ commits behind to modern DOSBox Staging, while preserving all 86 integration points.

### Key Numbers
- **86 integration points** to migrate
- **14 points (16.3%)** require DOSBox source modification
- **525-737 hours** estimated effort
- **8 implementation phases** over 16 weeks
- **~25,000 lines** of analysis documentation

### Core Constraints
1. DOSBox must build as static library (BOXER_INTEGRATED=ON)
2. All DOSBox modifications guarded by `#ifdef BOXER_INTEGRATED`
3. Standard DOSBox build must remain unaffected
4. INT-059 (emergency abort) must be <1μs overhead
5. Must preserve printer/parport functionality

---

## Directory Structure

```
boxer-upgrade/
├── README.md                  # This file
├── DECISION_LOG.md           # Architectural decisions tracker
├── PROGRESS.md               # Current status and metrics
│
├── instructions/             # Agent prompts and guidance
│   ├── MASTER_ORCHESTRATOR.md
│   ├── VALIDATION_GATES.md
│   └── phases/
│       ├── phase-1-foundation.md
│       ├── phase-2-lifecycle.md
│       └── ... (create as needed)
│
├── analysis/                 # Original analysis (READ-ONLY)
│   ├── 00-orchestrator/
│   │   ├── consolidated-strategy.md
│   │   └── ...
│   └── ...
│
├── progress/                 # Generated during implementation
│   ├── phase-1/
│   │   ├── OBJECTIVES.md
│   │   ├── tasks/
│   │   │   ├── TASK-1-1.md
│   │   │   └── ...
│   │   ├── validation/
│   │   └── PHASE_COMPLETE.md
│   └── ...
│
├── validation/               # Scripts and tests
│   ├── static-checks.sh
│   ├── build-test.sh
│   ├── consistency-check.sh
│   └── smoke-test/
│
├── src/                      # Source code (git submodules/clones)
│   ├── boxer/                # eduo/Boxer
│   ├── dosbox-staging/       # eduo/dosbox-staging (target)
│   └── dosbox-staging-legacy/# eduo/dosbox-staging-boxer (reference)
│
└── build/                    # Build outputs (gitignored)
    ├── dosbox-staging-normal/
    ├── dosbox-staging-boxer/
    └── boxer/
```

---

## How to Use This Project

### For Claude Agents

1. **Read the master orchestrator prompt** in `instructions/MASTER_ORCHESTRATOR.md`
2. **Check current state** in `PROGRESS.md`
3. **Review pending decisions** in `DECISION_LOG.md`
4. **Follow phase-specific prompts** in `instructions/phases/`
5. **Report work** in `progress/phase-X/tasks/`
6. **Run validation** using scripts in `validation/`

### For Human Developer

1. **Review agent reports** in `progress/phase-X/tasks/`
2. **Make decisions** when agents escalate (update `DECISION_LOG.md`)
3. **Approve phases** by completing Gate 3 review
4. **Track progress** in `PROGRESS.md`
5. **Resolve blockers** as they arise

---

## Phase Overview

### Phase 1: Foundation (60-80 hours)
- CMake build system setup
- Hook infrastructure headers
- Stub implementations
- First critical hook (INT-059)
- Link test validation

### Phase 2: Lifecycle (40-60 hours)
- Emulation start/stop control
- Emergency abort mechanism
- Window close handling

### Phase 3: Rendering (60-80 hours)
- SDL 2.0 / Metal integration
- Frame rendering callbacks
- Video mode switching

### Phase 4: Shell (120-160 hours)
- DOS shell lifecycle hooks
- Command interception
- Program launching

### Phase 5: File I/O (36-56 hours)
- File access control
- Security enforcement
- Drive management

### Phase 6: Parport (27-33 hours)
- Printer emulation migration
- ~4000 lines of code to port
- LPT DAC conflict resolution

### Phase 7: Input/Audio (52-78 hours)
- Keyboard, mouse, joystick
- MIDI passthrough
- Graphics modes

### Phase 8: Testing (160-240 hours)
- Comprehensive validation
- Performance optimization
- Bug fixes
- Release preparation

---

## Key Documents

### Must Read Before Starting
1. `analysis/00-orchestrator/consolidated-strategy.md`
   - Complete implementation plan
   - All stop conditions and success criteria

2. `analysis/01-current-integration/integration-overview.md`
   - All 86 integration points listed
   - Categories and priorities

3. `analysis/03-reintegration-analysis/unavoidable-modifications.md`
   - 14 Category C modifications detailed
   - Code samples and risk assessments

4. `DECISION_LOG.md`
   - Open decisions that block phases
   - Resolved decisions for consistency

### Reference During Implementation
- `instructions/MASTER_ORCHESTRATOR.md` - How agents coordinate
- `instructions/VALIDATION_GATES.md` - Quality assurance process
- `instructions/phases/phase-X.md` - Specific phase guidance

---

## Common Commands

### Build DOSBox (Normal)
```bash
cd build/dosbox-staging-normal
cmake ../../src/dosbox-staging/
cmake --build .
```

### Build DOSBox (Boxer Integration)
```bash
cd build/dosbox-staging-boxer
cmake -DBOXER_INTEGRATED=ON ../../src/dosbox-staging/
cmake --build .
```

### Run Static Analysis
```bash
./validation/static-checks.sh src/dosbox-staging
```

### Check Consistency
```bash
./validation/consistency-check.sh
```

### Run All Gates
```bash
./validation/run-all-gates.sh 1  # For Phase 1
```

---

## Important Notes

### What Agents Can't Do
- Access external websites or APIs
- Run compilation in early phases (no feedback)
- Make architectural decisions without human approval
- Skip validation gates

### What Humans Must Do
- Resolve decisions in `DECISION_LOG.md`
- Approve phases (Gate 3 review)
- Provide missing context when agents ask
- Track actual hours spent vs. estimates

### Error Recovery
- If agent reports a blocker → Human decides path forward
- If validation fails → Agent fixes before advancing
- If assumption invalidated → Reassess with human
- If phase takes >150% estimated time → Reassess scope

---

## Getting Help

### In-Project Resources
- `analysis/` - 25K lines of pre-analysis
- `instructions/` - Coordination guidance
- Agent task reports in `progress/`

### External Resources
- DOSBox Staging docs: https://www.dosbox-staging.org/
- CMake documentation: https://cmake.org/documentation/
- Original Boxer source for reference

### Escalation Path
1. Agent reports issue in task report
2. Orchestrator evaluates and attempts resolution
3. If architectural → Escalates to human
4. Human decides and updates `DECISION_LOG.md`
5. Agent proceeds with decision

---

## Success Criteria

### Technical Success
- [ ] All 86 integration points functional
- [ ] DOSBox builds as static library
- [ ] Boxer links and runs DOSBox
- [ ] <1μs overhead on INT-059
- [ ] All video modes working
- [ ] Printer functionality preserved

### Project Success
- [ ] Completed within 150% of estimate (787-1105 hours max)
- [ ] All phases properly validated
- [ ] Clean documentation trail
- [ ] Maintainable for future upstream merges

### Strategic Success
- [ ] Boxer can stay current with DOSBox Staging updates
- [ ] Reduced maintenance burden (<1 week/quarter for merges)
- [ ] Foundation for future enhancements

---

## License and Attribution

This project coordinates the upgrade of:
- **Boxer** - Original work by Alun Bestor, modifications by eduo
- **DOSBox Staging** - Modern continuation of DOSBox by the DOSBox Staging team

Analysis and coordination methodology developed with Claude (Anthropic).

All source code follows original project licenses.
