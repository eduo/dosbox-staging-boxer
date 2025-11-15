# Phase 4: Shell & Program Launching - Agent Tasks

**Phase Duration**: Weeks 7-8
**Total Estimated Hours**: 120-160 hours
**Goal**: DOS shell integration complete - command interception, program launching

**Prerequisites**: Phase 3 complete (rendering working)

**⚠️ HIGHEST COMPLEXITY PHASE** - Requires careful attention to detail

---

## IMPORTANT: Repository Structure

**Root**: `/home/user/dosbox-staging-boxer/boxer-upgrade/`

**Two SEPARATE Repositories**:
1. DOSBox Staging (`src/dosbox-staging/`) - Branch: `dosbox-boxer-upgrade-dosboxside`
2. Boxer (`src/boxer/`) - Branch: `boxer-dosbox-upgrade-boxerside`

**Phase 4 modifies**: Both DOSBox Staging (shell hooks) and Boxer (command handling, program launching)

---

## PHASE 4 OVERVIEW

By the end of Phase 4, you will have:
- Boxer can launch programs from UI
- DOS prompt works correctly
- Batch files execute properly
- Command interception functional
- Shell lifecycle fully integrated (15 callbacks)

**This is the MOST COMPLEX subsystem with the highest risk.**

---

## CRITICAL DECISION REQUIRED

Before starting Phase 4, **DEC-001 must be resolved**:
- **Direct Modification** vs **Subclass Approach** for shell integration

Check `DECISION_LOG.md` for human decision before proceeding.

---

## CRITICAL INTEGRATION POINTS (15 total)

From integration-overview.md:
- INT-023: shellWillStart
- INT-024: shellDidFinish
- INT-025: shellWillStartAutoexec
- INT-026: didReturnToShell
- INT-027: shellShouldRunCommand (MAJOR)
- INT-028: shellWillReadCommandInputFromHandle
- INT-029: shellDidReadCommandInputFromHandle
- INT-030: handleShellCommandInput (MAJOR)
- INT-031: hasPendingCommandsForShell
- INT-032: executeNextPendingCommandForShell
- INT-033: shellShouldDisplayStartupMessages
- INT-034: shellWillExecuteFileAtDOSPath
- INT-035: shellDidExecuteFileAtDOSPath
- INT-036: shellWillBeginBatchFile
- INT-037: shellDidEndBatchFile
- INT-038: shellShouldContinue (MAJOR)

---

## TASK 4-1: Shell Architecture Analysis

### Context
- **Phase**: 4
- **Estimated Hours**: 10-14 hours
- **Criticality**: CORE
- **Risk Level**: HIGH

### Objective
Understand target DOSBox shell architecture before modifying it.

### Prerequisites
- [ ] Phase 3 complete
- [ ] DEC-001 resolved (shell integration approach decided)

### Input Documents
1. `src/dosbox-staging/src/shell/shell.cpp`
   - DOS_Shell class definition
   - Run() method structure

2. `src/dosbox-staging/src/shell/shell_cmds.cpp`
   - Command execution

3. `src/dosbox-staging/src/shell/shell_misc.cpp`
   - Input handling, batch files

4. `analysis/03-reintegration-analysis/unavoidable-modifications.md` lines 265-420
   - Shell hook requirements

### Deliverables
1. **Analysis document**: `progress/phase-4/SHELL_ARCHITECTURE.md`
   - Class diagram of DOS_Shell
   - Method call sequences
   - State machine for shell lifecycle
   
2. **Hook placement map**: Where each of 15 hooks goes
   
3. **Documentation**: `progress/phase-4/tasks/TASK-4-1.md`

### Key Questions
1. How does DOS_Shell::Run() execute?
2. Where are commands parsed?
3. How are batch files processed?
4. What state does shell maintain?
5. How does AUTOEXEC.BAT execute?

### Success Criteria
- [ ] Complete understanding of shell execution flow
- [ ] All 15 hook points identified
- [ ] No architectural blockers found
- [ ] Integration strategy validated

---

## TASK 4-2: Basic Shell Lifecycle Hooks

### Context
- **Phase**: 4
- **Estimated Hours**: 18-24 hours
- **Criticality**: CORE
- **Risk Level**: MEDIUM

### Objective
Add the core lifecycle hooks: shellWillStart, shellDidFinish, shellShouldContinue.

### Prerequisites
- [ ] TASK 4-1 complete (architecture understood)

### Deliverables
1. **Modified**: `src/dosbox-staging/src/shell/shell.cpp`
   - Add hooks at Run() entry/exit
   - Add shouldContinue check in main loop
   - ~15-20 lines added
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement shellWillStart()
   - Implement shellDidFinish()
   - Implement shellShouldContinue()
   
3. **Test**: Shell lifecycle test
   
4. **Documentation**: `progress/phase-4/tasks/TASK-4-2.md`

### Implementation Pattern (Direct Modification)

```cpp
// In shell.cpp DOS_Shell::Run()
void DOS_Shell::Run() {
    #ifdef BOXER_INTEGRATED
    BOXER_HOOK_VOID(shellWillStart);
    #endif

    while (true) {
        #ifdef BOXER_INTEGRATED
        if (!BOXER_HOOK_BOOL(shellShouldContinue)) {
            break;
        }
        #endif
        
        // Existing shell loop code
        // ...
    }

    #ifdef BOXER_INTEGRATED
    BOXER_HOOK_VOID(shellDidFinish, exit_code);
    #endif
}
```

### Success Criteria
- [ ] Shell startup detected by Boxer
- [ ] Shell exit detected by Boxer
- [ ] Boxer can control shell continuation
- [ ] No crashes or hangs

---

## TASK 4-3: AUTOEXEC.BAT Hooks

### Context
- **Phase**: 4
- **Estimated Hours**: 12-16 hours
- **Criticality**: MAJOR
- **Risk Level**: MEDIUM

### Objective
Hook AUTOEXEC.BAT execution so Boxer can monitor startup sequence.

### Prerequisites
- [ ] TASK 4-2 complete (basic lifecycle works)

### Deliverables
1. **Modified**: `src/dosbox-staging/src/shell/shell.cpp`
   - Add hook before AUTOEXEC runs
   - Add hook to suppress startup messages if desired
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement shellWillStartAutoexec()
   - Implement shellShouldDisplayStartupMessages()
   
3. **Documentation**: `progress/phase-4/tasks/TASK-4-3.md`

### Use Cases
1. Boxer needs to know when AUTOEXEC starts to suppress UI
2. Boxer may want to hide DOS startup messages
3. Boxer may inject additional commands before AUTOEXEC

### Success Criteria
- [ ] AUTOEXEC start detected
- [ ] Startup messages controllable
- [ ] No interference with normal execution

---

## TASK 4-4: Command Interception

### Context
- **Phase**: 4
- **Estimated Hours**: 24-32 hours
- **Criticality**: MAJOR
- **Risk Level**: HIGH

### Objective
Intercept DOS commands so Boxer can react to them (MOUNT, CD, program launches).

### Prerequisites
- [ ] TASK 4-3 complete (AUTOEXEC hooks)
- [ ] Understand command parsing flow

### Deliverables
1. **Modified**: `src/dosbox-staging/src/shell/shell_cmds.cpp`
   - Add hook before command execution
   - Add hook after command execution
   - Pass command string and arguments
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement shellShouldRunCommand()
   - Implement handleShellCommandInput()
   - Parse commands and dispatch to Boxer
   
3. **Modified**: `src/boxer/Boxer/BXEmulator+BXShell.mm`
   - Handle MOUNT commands
   - Handle CD commands
   - Update UI based on commands
   
4. **Test**: Command interception test suite
   
5. **Documentation**: `progress/phase-4/tasks/TASK-4-4.md`

### Implementation Pattern

```cpp
// In shell_cmds.cpp
bool DOS_Shell::Execute(char* name, char* args) {
    #ifdef BOXER_INTEGRATED
    char full_cmd[CMD_MAXLINE];
    snprintf(full_cmd, sizeof(full_cmd), "%s %s", name, args);
    
    // Let Boxer decide if command should run
    bool allow = BOXER_HOOK_BOOL(shellShouldRunCommand, full_cmd);
    if (!allow) {
        return true; // Boxer handled it
    }
    #endif
    
    // Execute command normally
    bool result = DoCommand(name, args);
    
    #ifdef BOXER_INTEGRATED
    BOXER_HOOK_VOID(handleShellCommandInput, full_cmd, result ? 0 : 1);
    #endif
    
    return result;
}
```

### Decision Points - STOP if:

1. **Command parsing complexity**: Too many edge cases
   - Options: A) Parse in DOSBox, B) Parse in Boxer, C) Pass raw string
   - Report: What's causing issues

2. **Command blocking**: Should Boxer be able to block commands?
   - Report: Use cases, security implications

### Success Criteria
- [ ] All commands intercepted
- [ ] Boxer can react to MOUNT
- [ ] Boxer can react to CD
- [ ] Program launches tracked
- [ ] No command corruption

---

## TASK 4-5: Program Execution Tracking

### Context
- **Phase**: 4
- **Estimated Hours**: 20-24 hours
- **Criticality**: MAJOR
- **Risk Level**: MEDIUM

### Objective
Track when DOS programs are executed and finished.

### Prerequisites
- [ ] TASK 4-4 complete (commands intercepted)

### Deliverables
1. **Modified**: `src/dosbox-staging/src/shell/shell_misc.cpp`
   - Add hook before program execution
   - Add hook after program execution
   - Pass program path and arguments
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement shellWillExecuteFileAtDOSPath()
   - Implement shellDidExecuteFileAtDOSPath()
   
3. **Modified**: `src/boxer/Boxer/BXEmulator+BXShell.mm`
   - Apply game-specific settings
   - Track which program is running
   - Update UI
   
4. **Documentation**: `progress/phase-4/tasks/TASK-4-5.md`

### Use Cases
1. User types GAME.EXE → Boxer applies save game settings
2. Program finishes → Boxer returns to launcher UI
3. Program crashes → Boxer handles error gracefully

### Success Criteria
- [ ] Program start/end detected
- [ ] Program path correctly parsed
- [ ] Exit code captured
- [ ] UI updates appropriately

---

## TASK 4-6: Batch File Monitoring

### Context
- **Phase**: 4
- **Estimated Hours**: 12-16 hours
- **Criticality**: MINOR
- **Risk Level**: LOW

### Objective
Monitor batch file execution for debugging and game configuration.

### Prerequisites
- [ ] TASK 4-5 complete (program tracking)

### Deliverables
1. **Modified**: `src/dosbox-staging/src/shell/shell_batch.cpp`
   - Add hook when batch file starts
   - Add hook when batch file ends
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement shellWillBeginBatchFile()
   - Implement shellDidEndBatchFile()
   
3. **Documentation**: `progress/phase-4/tasks/TASK-4-6.md`

### Success Criteria
- [ ] Batch file start/end detected
- [ ] Nested batch files handled
- [ ] No interference with execution

---

## TASK 4-7: Command Injection

### Context
- **Phase**: 4
- **Estimated Hours**: 16-20 hours
- **Criticality**: MAJOR
- **Risk Level**: MEDIUM

### Objective
Allow Boxer to inject commands into DOS shell (for game launching).

### Prerequisites
- [ ] TASK 4-6 complete (all monitoring in place)

### Deliverables
1. **Modified**: `src/dosbox-staging/src/shell/shell.cpp`
   - Add hook to check for pending commands
   - Add hook to execute pending command
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement hasPendingCommandsForShell()
   - Implement executeNextPendingCommandForShell()
   - Queue management
   
3. **Modified**: `src/boxer/Boxer/BXEmulator+BXShell.mm`
   - Queue commands when user launches game
   - Execute CD, then GAME.EXE
   
4. **Test**: Command injection test
   
5. **Documentation**: `progress/phase-4/tasks/TASK-4-7.md`

### Implementation Pattern

```objc
// In BXEmulator+BXShell.mm
- (void)launchProgramAtPath:(NSString *)path {
    // Queue commands
    [self queueCommand:@"C:"];
    [self queueCommand:[NSString stringWithFormat:@"CD %@", [path stringByDeletingLastPathComponent]]];
    [self queueCommand:[path lastPathComponent]];
}

// In BoxerDelegate
bool hasPendingCommandsForShell() {
    return [_owner hasPendingCommands];
}

const char* executeNextPendingCommandForShell() {
    return [[_owner dequeueCommand] UTF8String];
}
```

### Success Criteria
- [ ] Commands can be queued from Boxer UI
- [ ] Commands execute in order
- [ ] Game launches correctly
- [ ] No race conditions

---

## TASK 4-8: Integration Testing

### Context
- **Phase**: 4
- **Estimated Hours**: 16-20 hours
- **Criticality**: MAJOR
- **Risk Level**: LOW

### Objective
Comprehensive testing of all 15 shell hooks working together.

### Prerequisites
- [ ] TASK 4-7 complete (all hooks implemented)

### Deliverables
1. **Test suite**: `validation/contracts/test_shell_integration.cpp`
   - Test each hook individually
   - Test hooks in combination
   - Test error conditions
   
2. **Integration test**: End-to-end game launch test
   
3. **Documentation**: `progress/phase-4/tasks/TASK-4-8.md`

### Test Scenarios
1. Cold boot → AUTOEXEC → shell prompt
2. Launch program from Boxer UI
3. User types command at DOS prompt
4. Batch file execution with nested batch
5. Program crashes during execution
6. User types EXIT at DOS prompt

### Success Criteria
- [ ] All 15 hooks functional
- [ ] No missed callbacks
- [ ] No duplicate callbacks
- [ ] State consistency maintained

---

## PHASE 4 COMPLETION CHECKLIST

### Shell Lifecycle ✅
- [ ] shellWillStart works
- [ ] shellDidFinish works
- [ ] shellShouldContinue works
- [ ] AUTOEXEC hooks work

### Command Handling ✅
- [ ] Command interception works
- [ ] MOUNT commands detected
- [ ] CD commands detected
- [ ] Command injection works

### Program Execution ✅
- [ ] Program launch detection
- [ ] Program exit detection
- [ ] Exit code captured
- [ ] Game settings applied

### Batch Files ✅
- [ ] Batch start/end detected
- [ ] Nested batches handled
- [ ] No execution errors

### Integration ✅
- [ ] All 15 hooks working together
- [ ] UI updates correctly
- [ ] No race conditions
- [ ] No crashes

**When all boxes checked, Phase 4 is complete. Ready for Phase 5 (File I/O).**

---

## ESTIMATED TIME BREAKDOWN

- TASK 4-1: Architecture Analysis - 10-14 hours
- TASK 4-2: Basic Lifecycle Hooks - 18-24 hours
- TASK 4-3: AUTOEXEC Hooks - 12-16 hours
- TASK 4-4: Command Interception - 24-32 hours
- TASK 4-5: Program Execution Tracking - 20-24 hours
- TASK 4-6: Batch File Monitoring - 12-16 hours
- TASK 4-7: Command Injection - 16-20 hours
- TASK 4-8: Integration Testing - 16-20 hours

**Total**: 128-166 hours (~120-160 planned)

**Calendar time**: 2-3 weeks (this is the longest phase)

---

## HIGH-RISK AREAS

1. **Shell refactoring in upstream**: Target DOSBox actively develops shell
2. **State consistency**: 15 callbacks must maintain consistent state
3. **Command parsing**: Edge cases in DOS command syntax
4. **Race conditions**: Multiple threads accessing shell state
5. **Merge conflicts**: Highest risk of upstream conflicts

**Mitigation**: Comprehensive testing, careful state management, frequent upstream checks.