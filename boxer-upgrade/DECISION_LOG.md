# DECISION LOG - Boxer DOSBox Staging Upgrade

This document tracks all architectural and strategic decisions made during the upgrade project. Agents check this before making decisions. Humans update this after reviewing escalations.

---

## OPEN DECISIONS (Awaiting Human Input)

### DEC-001: Shell Integration Approach
- **Date Raised**: [To be filled when Phase 4 starts]
- **Raised By**: Analysis Phase
- **Affects**: Phase 4 (120-160 hours), ongoing maintenance
- **Blocking**: Phase 4 start

**Issue**: Target DOSBox has refactored shell structure with no lifecycle hooks. Need to add 15 shell callbacks.

**Options**:
A. **Direct Modification** to DOS_Shell class
   - Pros: Simpler implementation, faster Phase 4
   - Cons: Higher merge conflict risk, tighter coupling
   - Estimated effort: 120-160 hours

B. **Subclass Approach** (BoxerShell : DOS_Shell)
   - Pros: Cleaner separation, lower merge conflict risk
   - Cons: More upfront work, state access complexity
   - Estimated effort: 160-200 hours

C. **Hybrid** (Direct for lifecycle, subclass for commands)
   - Pros: Balance of both
   - Cons: Inconsistent pattern, harder to maintain
   - Estimated effort: 140-180 hours

**Analysis Recommendation**: Option A (Direct Modification)

**Human Decision**: [PENDING - Decide before Phase 4]

---

### DEC-002: LPT DAC Conflict Resolution
- **Date Raised**: Analysis Phase
- **Raised By**: Agent 7 (Parport Migration)
- **Affects**: Phase 6 (27-33 hours), user-facing behavior
- **Blocking**: Phase 6 start

**Issue**: Parallel port can be used for either printer OR LPT DAC audio, not both simultaneously.

**Options**:
A. **Runtime Detection** with user override
   - Detect what's connected, allow user to force choice
   - Pros: Most flexible, preserves all features
   - Cons: More complex, config UI needed
   - Effort: +4-6 hours

B. **Compile-time Flag**
   - Define at build time which is supported
   - Pros: Simpler, predictable behavior
   - Cons: Loses flexibility, two build variants?
   - Effort: +0 hours

C. **Printer-only** (remove LPT DAC support)
   - Pros: Simplest, printer is more commonly used
   - Cons: Power users lose audio feature
   - Effort: -2 hours

D. **LPT DAC-only** (remove printer support)
   - Pros: Power users happy
   - Cons: Printer is flagship Boxer feature
   - Effort: -10 hours (less code to migrate)

**Analysis Recommendation**: Option A (Runtime Detection)

**Human Decision**: [PENDING - Decide before Phase 6]

---

### DEC-003: Paste Buffer Implementation Location
- **Date Raised**: Analysis Phase
- **Raised By**: Agent 7 (Input Handling)
- **Affects**: Phase 7 (12-18 hours), code ownership
- **Blocking**: Phase 7 start

**Issue**: Target DOSBox has no paste buffer. Need to implement clipboard paste functionality.

**Options**:
A. **Boxer-Side** implementation
   - Boxer intercepts paste, injects keycodes into DOSBox
   - Pros: No DOSBox modification, moves to Category D
   - Cons: More complex Boxer code
   - Effort: 12-18 hours

B. **DOSBox-Side** implementation
   - Add paste buffer to target DOSBox source
   - Pros: Simpler for Boxer
   - Cons: DOSBox modification, Category C point
   - Effort: 8-12 hours

C. **Hybrid** (queue in Boxer, consume in DOSBox)
   - Pros: Shared responsibility
   - Cons: API coupling, complexity
   - Effort: 16-24 hours

**Analysis Recommendation**: Option A (Boxer-Side)

**Human Decision**: [PENDING - Decide before Phase 7]

---

## RESOLVED DECISIONS

### DEC-000: Library Type (STATIC vs SHARED)
- **Date Resolved**: 2025-11-14
- **Decided By**: Analysis document + pre-validation
- **Decision**: **STATIC library**
- **Rationale**: 
  - Simpler linking for Xcode project
  - No runtime dependency on library location
  - Consistent with Boxer's existing structure
  - Matches analysis recommendation

### DEC-010: Build System (CMake Structure)
- **Date Resolved**: 2025-11-14 (implicit in analysis)
- **Decision**: **Add BOXER_INTEGRATED option to existing CMakeLists.txt**
- **Rationale**:
  - Minimizes upstream divergence
  - Follows CMake best practices
  - All changes guarded by if(BOXER_INTEGRATED)

### DEC-011: Preprocessor Guard Strategy
- **Date Resolved**: 2025-11-14 (implicit in analysis)
- **Decision**: **Use #ifdef BOXER_INTEGRATED for all DOSBox modifications**
- **Rationale**:
  - Standard pattern for conditional compilation
  - Clear what is Boxer-specific vs. upstream
  - Easy to grep/track all modifications

---

## DECISION CATEGORIES

### Architecture (ARCH-)
Major structural decisions affecting multiple components.
- ARCH-001: Hook infrastructure pattern (IBoxerDelegate interface) - RESOLVED
- ARCH-002: Shell integration approach - OPEN (DEC-001)
- ARCH-003: File I/O interception strategy - TO BE DETERMINED

### Implementation (IMPL-)
Specific implementation choices within a component.
- IMPL-001: Library type selection - RESOLVED (DEC-000)
- IMPL-002: CMake option naming - RESOLVED (BOXER_INTEGRATED)
- IMPL-003: Hook macro design - TO BE DETERMINED

### Performance (PERF-)
Decisions affecting runtime performance.
- PERF-001: INT-059 frequency check - RESOLVED (<1μs requirement)
- PERF-002: Atomic memory ordering - TO BE DETERMINED
- PERF-003: SDL event batching - TO BE DETERMINED

### Compatibility (COMPAT-)
Decisions affecting upstream compatibility.
- COMPAT-001: Merge strategy - TO BE DETERMINED
- COMPAT-002: API versioning - TO BE DETERMINED
- COMPAT-003: Feature flag support - TO BE DETERMINED

---

## DECISION-MAKING GUIDELINES

### When Agent Must Escalate
- Decision affects >4 hours of work
- Multiple valid options with significant tradeoffs
- Changes existing resolved decision
- Introduces new pattern not in analysis
- Affects performance or compatibility

### When Agent Can Decide
- Code style within established pattern
- Variable naming following conventions
- Comment placement and content
- Error message text
- Order of operations within task

### Decision Documentation Requirements
- Clear problem statement
- All options considered
- Pros/cons for each option
- Estimated effort impact
- Recommendation with justification
- Human decision with date

---

## DECISION IMPACT TRACKING

After a decision is made, track its impact:

### DEC-000 Impact (Static Library)
- Files affected: CMakeLists.txt, build scripts
- Hours saved/added: 0 (baseline choice)
- Complications discovered: None yet
- Rework required: None

### DEC-001 Impact (TBD)
- Files affected: [To be determined]
- Hours saved/added: [To be determined]
- Complications discovered: [To be tracked]
- Rework required: [To be tracked]

---

## TEMPLATE FOR NEW DECISIONS

```markdown
### DEC-XXX: [Short Title]
- **Date Raised**: [ISO date]
- **Raised By**: [Agent/Human]
- **Affects**: Phase [X], [specific components]
- **Blocking**: [What can't proceed without this]

**Issue**: [Clear description of the decision needed]

**Options**:
A. **[Option Name]**
   - [Description]
   - Pros: [List]
   - Cons: [List]
   - Effort: [Hours impact]

B. **[Option Name]**
   - [Description]
   - Pros: [List]
   - Cons: [List]
   - Effort: [Hours impact]

**Analysis Recommendation**: [Option X] because [reasoning]

**Human Decision**: [PENDING / Option X chosen on DATE]

**Post-Decision Notes**: [Any implementation notes]
```

---

## DECISION REVIEW SCHEDULE

- **Phase 1 End**: Review DEC-000 through DEC-011 (foundation choices)
- **Phase 3 End**: Review pending decisions, decide DEC-001 (shell)
- **Phase 5 End**: Review pending decisions, decide DEC-002 (LPT DAC)
- **Phase 7 Start**: Ensure DEC-003 decided (paste buffer)
- **Phase 8**: Final review of all decisions, lessons learned

---

## VERSION HISTORY

- 2025-11-14: Initial document created from analysis phase
- [Date]: [Changes made]
