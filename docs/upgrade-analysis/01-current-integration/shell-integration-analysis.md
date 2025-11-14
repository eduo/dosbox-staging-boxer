# Shell Integration Analysis

**Agent**: Agent 1B.3 - Shell Integration
**Created**: 2025-11-14T13:32:00Z
**Status**: Completed
**Dependencies**: Agent 1A

## Summary

**Critical Finding**: All 15 shell integration callbacks are MISSING from DOSBox Staging. The target codebase has removed all Boxer-specific hooks, requiring complete reimplementation of shell integration functionality. The shell architecture has been significantly refactored with improved C++ patterns (std::string_view, std::optional, std::stack for batch files) but lacks any extension points for external integration.

**Compatibility**: 0 of 15 integration points have direct equivalents
**Migration Effort**: 120-160 hours (requires new integration architecture)
**Risk Level**: HIGH - Complete rearchitecture required

---

## Legacy Shell Architecture

### DOS_Shell Class Structure

**Location**: `/home/user/dosbox-staging-boxer/src/shell/shell.cpp`

**Key Components**:
- **DOS_Shell** class: Main shell implementation
- **BatchFile** class: Batch file processing with filename tracking
- **currentShell** global variable: Tracks active shell instance (line 40)
- **first_shell** global variable: Tracks primary shell instance (line 37)
- **echo** member: Controls command echoing
- **bf** member: Shared pointer to current batch file
- **input_handle**: File handle for command input

### Shell Execution Flow

```
1. SHELL_Init() → Creates first_shell
2. DOS_Shell::Run() → Main shell loop
   ├─> boxer_shellWillStart() [INT-023]
   ├─> boxer_shellWillStartAutoexec() [INT-025]
   ├─> boxer_shellShouldDisplayStartupMessages() [INT-033]
   ├─> Command Loop:
   │   ├─> boxer_hasPendingCommandsForShell() [INT-031]
   │   ├─> boxer_executeNextPendingCommandForShell() [INT-032]
   │   ├─> boxer_didReturnToShell() [INT-026]
   │   ├─> InputCommand()
   │   │   ├─> boxer_shellWillReadCommandInputFromHandle() [INT-028]
   │   │   ├─> boxer_shellDidReadCommandInputFromHandle() [INT-029]
   │   │   └─> boxer_handleShellCommandInput() [INT-030]
   │   ├─> boxer_shellShouldContinue() [INT-038]
   │   └─> ParseLine()
   └─> boxer_shellDidFinish() [INT-024]

3. DoCommand() → Command execution
   └─> boxer_shellShouldRunCommand() [INT-027]

4. Execute() → Program/batch execution
   ├─> boxer_shellWillBeginBatchFile() [INT-036]
   ├─> boxer_shellWillExecuteFileAtDOSPath() [INT-034]
   └─> boxer_shellDidExecuteFileAtDOSPath() [INT-035]

5. BatchFile::~BatchFile() → Cleanup
   └─> boxer_shellDidEndBatchFile() [INT-037]
```

### Callback Integration Points

**Purpose**: These callbacks allow Boxer to:
1. **Monitor shell lifecycle** - Track when shells start/stop
2. **Inject commands** - Insert Boxer commands into DOS shell execution
3. **Control execution** - Interrupt or redirect shell behavior
4. **Track program launches** - Monitor which DOS programs are executed
5. **Manage autoexec** - Control AUTOEXEC.BAT processing
6. **Handle input** - Intercept and modify command input
7. **Control display** - Manage what startup messages are shown

---

## Target Shell Architecture

### File Structure

**Location**: `/home/user/dosbox-staging/src/shell/`

```
shell.cpp           (1354 lines) - Main shell implementation
shell_cmds.cpp      (large)      - Command implementations
shell_misc.cpp      (673 lines)  - Input handling and utilities
shell_batch.cpp     (168 lines)  - Batch file processing
shell_history.cpp   (new)        - Command history management
autoexec.cpp        (new)        - AUTOEXEC.BAT management
command_line.cpp    (new)        - Command line parsing
file_reader.cpp     (new)        - File reading abstraction
```

### Architecture Changes

**Major Improvements**:
1. **Modern C++ Patterns**:
   - `std::string_view` for string parameters (zero-copy)
   - `std::optional` for optional returns
   - `std::stack<BatchFile>` instead of shared pointers
   - `std::unique_ptr` for ownership management

2. **Better Separation of Concerns**:
   - `ShellHistory` class for command history
   - `CommandPrompt` class for prompt handling
   - `FileReader` abstraction for file input
   - `Environment` abstraction (PSP-based)

3. **Improved Redirection**:
   - Regex-based redirection parsing (line 75-187)
   - Pipe support with temporary files
   - Better error handling

4. **No Global State**:
   - No `currentShell` global variable
   - No direct access to batch file internals
   - Batch files stored in private stack

**Key Structural Differences**:

| Feature | Legacy (Boxer DOSBox) | Target (DOSBox Staging) |
|---------|----------------------|-------------------------|
| Batch Files | `shared_ptr<BatchFile> bf` | `std::stack<BatchFile> batchfiles` |
| Shell Tracking | `currentShell` global | No tracking |
| First Shell | `first_shell` global | Local in init function |
| Input Loop | `InputCommand()` with callbacks | `ReadCommand()` with `CommandPrompt` |
| Program Execution | `Execute()` with callbacks | `ExecuteProgram()` streamlined |
| Batch Cleanup | Destructor with callback | Automatic via stack pop |

### DOS_Shell Class (Target)

**Location**: `/home/user/dosbox-staging/src/shell/shell.h` (inferred)

**Refactored Structure**:
```cpp
class DOS_Shell {
    // No more currentShell tracking
    // No exposed batch file access
    std::stack<BatchFile> batchfiles;  // Private
    std::shared_ptr<ShellHistory> history;

    void Run();
    void RunBatchFile();
    std::string ReadCommand();
    bool ExecuteProgram(std::string_view name, std::string_view args);
    std::string ResolvePath(std::string_view name) const;
};
```

**Missing Integration Points**:
- No shell lifecycle callbacks
- No command injection mechanism
- No input interception hooks
- No program execution tracking
- No batch file lifecycle callbacks
- No external access to shell state

---

## Integration Point Analysis

### INT-023: boxer_shellWillStart

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/shell/shell.cpp:342`
- **Context**:
```cpp
void DOS_Shell::Run()
{
    boxer_shellWillStart(this);  // <-- CALLBACK
    DOS_Shell *previousShell = currentShell;
    currentShell = this;
```
- **Purpose**: Notifies Boxer when a shell instance starts execution
- **Signature**: `void boxer_shellWillStart(DOS_Shell* shell)`
- **Usage**: Boxer tracks shell lifecycle, initializes per-shell state

**Target Equivalent**:
- **Status**: MISSING
- **Location**: NOT FOUND
- **Changes**: `Run()` method exists at shell.cpp:425 but has no callback

**Compatibility**: MISSING

**Migration Strategy**:
1. Add virtual hook point to DOS_Shell::Run() entry
2. Create callback registration system
3. Implement shell tracking mechanism (may need to restore global or use registry pattern)
4. Estimated effort: 8 hours

---

### INT-024: boxer_shellDidFinish

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/shell/shell.cpp:364,444`
- **Context**:
```cpp
// Location 1: /C command completion
currentShell = previousShell;
boxer_shellDidFinish(this);  // <-- CALLBACK
return;

// Location 2: Main shell exit
currentShell = previousShell;
boxer_shellDidFinish(this);  // <-- CALLBACK
```
- **Purpose**: Notifies Boxer when shell execution completes (2 exit points)
- **Signature**: `void boxer_shellDidFinish(DOS_Shell* shell)`
- **Usage**: Boxer cleanup, state restoration

**Target Equivalent**:
- **Status**: MISSING (both locations)
- **Location**: NOT FOUND
- **Changes**: Run() exits at multiple points (lines 446, 488) with no callbacks

**Compatibility**: MISSING

**Migration Strategy**:
1. Add virtual hook to Run() exit points
2. Consider RAII pattern with destructor cleanup
3. Handle both /C mode and normal exit
4. Estimated effort: 6 hours

---

### INT-025: boxer_shellWillStartAutoexec

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/shell/shell.cpp:371`
- **Context**:
```cpp
if (cmd->FindString("/INIT",line,true)) {
    boxer_shellWillStartAutoexec(this);  // <-- CALLBACK

    const bool wants_welcome_banner = ...
```
- **Purpose**: Notifies Boxer before AUTOEXEC.BAT processing begins
- **Signature**: `void boxer_shellWillStartAutoexec(DOS_Shell* shell)`
- **Usage**: Boxer prepares for autoexec, may inject commands

**Target Equivalent**:
- **Status**: MISSING
- **Location**: `/home/user/dosbox-staging/src/shell/shell.cpp:449` has /INIT handling
- **Changes**: No callback before AUTOEXEC processing

**Compatibility**: MISSING

**Migration Strategy**:
1. Add callback before line 474 (ParseLine for /INIT)
2. Integrate with autoexec module (autoexec.cpp)
3. Estimated effort: 4 hours

---

### INT-026: boxer_didReturnToShell

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/shell/shell.cpp:428`
- **Context**:
```cpp
} else {
    boxer_didReturnToShell(this);  // <-- CALLBACK
    if (echo) ShowPrompt();
    InputCommand(input_line);
```
- **Purpose**: Notifies Boxer when control returns to DOS prompt (no batch file active)
- **Signature**: `void boxer_didReturnToShell(DOS_Shell* shell)`
- **Usage**: Boxer updates UI, may inject queued commands

**Target Equivalent**:
- **Status**: MISSING
- **Location**: Target has similar structure at shell.cpp:483-484 but no callback
- **Changes**: `if (!batchfiles.empty())` check exists but no notification

**Compatibility**: MISSING

**Migration Strategy**:
1. Add callback when batchfiles stack is empty and showing prompt
2. Insert before ShowPrompt() call at line 484
3. Estimated effort: 3 hours

---

### INT-027: boxer_shellShouldRunCommand

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/shell/shell_cmds.cpp:182`
- **Context**:
```cpp
if (!boxer_shellShouldRunCommand(this, cmd_buffer, line))
    return;  // <-- Boxer intercepts command

/* Check the internal list */
if (execute_shell_cmd(cmd_buffer, line))
```
- **Purpose**: Allows Boxer to intercept and handle commands before DOSBox processes them
- **Signature**: `bool boxer_shellShouldRunCommand(DOS_Shell* shell, char* cmd, char* args)`
- **Usage**: Boxer handles custom commands (like drive operations), returns false to skip DOSBox processing

**Target Equivalent**:
- **Status**: MISSING
- **Location**: DoCommand() method exists but no callback before command execution
- **Changes**: Command processing is streamlined, no interception point

**Compatibility**: MISSING

**Migration Strategy**:
1. Add virtual method for command interception in DoCommand()
2. Insert before internal command lookup
3. Return bool to indicate whether to continue processing
4. Estimated effort: 6 hours

---

### INT-028: boxer_shellWillReadCommandInputFromHandle

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/shell/shell_misc.cpp:75`
- **Context**:
```cpp
while (size && !shutdown_requested) {
    dos.echo=false;

    boxer_shellWillReadCommandInputFromHandle(this, input_handle);  // <-- CALLBACK
    while(boxer_continueListeningForKeyEvents() && !DOS_ReadFile(input_handle,&c,&n)) {
```
- **Purpose**: Notifies Boxer before reading keyboard input
- **Signature**: `void boxer_shellWillReadCommandInputFromHandle(DOS_Shell* shell, uint16_t handle)`
- **Usage**: Boxer may inject input or prepare for input handling

**Target Equivalent**:
- **Status**: MISSING
- **Location**: ReadCommand() at shell_misc.cpp:90 has input loop but no callback
- **Changes**: Completely refactored input handling with CommandPrompt class

**Compatibility**: MISSING

**Migration Strategy**:
1. Add callback in ReadCommand() before DOS_ReadFile() at line 125
2. Coordinate with CommandPrompt class
3. Estimated effort: 8 hours (complex input handling)

---

### INT-029: boxer_shellDidReadCommandInputFromHandle

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/shell/shell_misc.cpp:82`
- **Context**:
```cpp
        LOG(LOG_MISC,LOG_ERROR)("Reopening the input handle. This is a bug!");
    }
    boxer_shellDidReadCommandInputFromHandle(this, input_handle);  // <-- CALLBACK

    if (!boxer_shellShouldContinue(this))
```
- **Purpose**: Notifies Boxer after reading keyboard input
- **Signature**: `void boxer_shellDidReadCommandInputFromHandle(DOS_Shell* shell, uint16_t handle)`
- **Usage**: Boxer processes input, may modify it

**Target Equivalent**:
- **Status**: MISSING
- **Location**: No callback after DOS_ReadFile() at shell_misc.cpp:125
- **Changes**: Input processing is immediate and integrated

**Compatibility**: MISSING

**Migration Strategy**:
1. Add callback after DOS_ReadFile() at line 131
2. Allow modification of input data before processing
3. Estimated effort: 6 hours

---

### INT-030: boxer_handleShellCommandInput

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/shell/shell_misc.cpp:90`
- **Context**:
```cpp
    bool executeImmediately = false;
    if (boxer_handleShellCommandInput(this, line, &str_index, &executeImmediately))
    {
        if (executeImmediately)
        {
            size = 0;
            break;  // Exit input loop
```
- **Purpose**: Allows Boxer to handle and modify command input, optionally executing immediately
- **Signature**: `bool boxer_handleShellCommandInput(DOS_Shell* shell, char* line, size_t* cursor_pos, bool* exec_now)`
- **Usage**: Boxer intercepts special key combinations, injects commands, controls execution flow

**Target Equivalent**:
- **Status**: MISSING
- **Location**: ReadCommand() has no input interception mechanism
- **Changes**: Input loop is fully self-contained in CommandPrompt

**Compatibility**: MISSING

**Migration Strategy**:
1. Add virtual input handler hook in ReadCommand() input loop
2. Allow line modification and immediate execution
3. Complex due to CommandPrompt encapsulation
4. Estimated effort: 12 hours (HIGH complexity)

---

### INT-031: boxer_hasPendingCommandsForShell

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/shell/shell.cpp:408`
- **Context**:
```cpp
do {
    if (boxer_hasPendingCommandsForShell(this))  // <-- CALLBACK
    {
        boxer_executeNextPendingCommandForShell(this);
    }
    else if (bf){
```
- **Purpose**: Checks if Boxer has queued commands to inject
- **Signature**: `bool boxer_hasPendingCommandsForShell(DOS_Shell* shell)`
- **Usage**: Boxer maintains command queue, injects commands at appropriate times

**Target Equivalent**:
- **Status**: MISSING
- **Location**: Main loop at shell.cpp:480 has no command queue mechanism
- **Changes**: No external command injection system

**Compatibility**: MISSING

**Migration Strategy**:
1. Add command queue system to DOS_Shell
2. Check queue in main loop before batch file processing
3. Requires shell state management
4. Estimated effort: 10 hours

---

### INT-032: boxer_executeNextPendingCommandForShell

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/shell/shell.cpp:410`
- **Context**:
```cpp
if (boxer_hasPendingCommandsForShell(this))
{
    boxer_executeNextPendingCommandForShell(this);  // <-- CALLBACK
}
```
- **Purpose**: Executes the next command from Boxer's queue
- **Signature**: `void boxer_executeNextPendingCommandForShell(DOS_Shell* shell)`
- **Usage**: Boxer pops command from queue and executes it

**Target Equivalent**:
- **Status**: MISSING
- **Location**: NOT FOUND
- **Changes**: No command execution injection mechanism

**Compatibility**: MISSING

**Migration Strategy**:
1. Implement with INT-031 as part of command queue system
2. Execute queued command via ParseLine()
3. Estimated effort: included in INT-031 (10 hours total)

---

### INT-033: boxer_shellShouldDisplayStartupMessages

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/shell/shell.cpp:377`
- **Context**:
```cpp
const bool wants_welcome_banner = (control->GetStartupVerbosity() >=
                                  Verbosity::Medium) ||
                                  boxer_shellShouldDisplayStartupMessages(this);  // <-- CALLBACK
```
- **Purpose**: Allows Boxer to override startup message display
- **Signature**: `bool boxer_shellShouldDisplayStartupMessages(DOS_Shell* shell)`
- **Usage**: Boxer can force banner display or hide it

**Target Equivalent**:
- **Status**: MISSING
- **Location**: Banner display at shell.cpp:450-472 uses only GetStartupVerbosity()
- **Changes**: No external control over banner display

**Compatibility**: MISSING

**Migration Strategy**:
1. Add virtual method for banner display control
2. Insert check before line 452
3. Estimated effort: 2 hours (LOW complexity)

---

### INT-034: boxer_shellWillExecuteFileAtDOSPath

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/shell/shell_misc.cpp:557`
- **Context**:
```cpp
if(strcasecmp(extension, ".exe") !=0) return false;
}
boxer_shellWillExecuteFileAtDOSPath(this, canonicalPath, args);  // <-- CALLBACK
//--End of modifications
/* Run the .exe or .com file from the shell */
```
- **Purpose**: Notifies Boxer before executing .COM or .EXE file
- **Signature**: `void boxer_shellWillExecuteFileAtDOSPath(DOS_Shell* shell, const char* path, const char* args)`
- **Usage**: Boxer tracks program launches, may modify execution

**Target Equivalent**:
- **Status**: MISSING
- **Location**: ExecuteProgram() at shell_misc.cpp:490, runs programs via run_binary_executable() (line 527)
- **Changes**: No callback before binary execution

**Compatibility**: MISSING

**Migration Strategy**:
1. Add callback in ExecuteProgram() before run_binary_executable() call
2. Pass resolved path and arguments
3. Estimated effort: 5 hours

---

### INT-035: boxer_shellDidExecuteFileAtDOSPath

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/shell/shell_misc.cpp:656`
- **Context**:
```cpp
reg_sp+=0x200;

//--Added 2010-01-21 by Alun Bestor to let Boxer track the executed program
boxer_shellDidExecuteFileAtDOSPath(this, canonicalPath);  // <-- CALLBACK
//--End of modifications
}
return true; //Executable started
```
- **Purpose**: Notifies Boxer after .COM or .EXE execution completes
- **Signature**: `void boxer_shellDidExecuteFileAtDOSPath(DOS_Shell* shell, const char* path)`
- **Usage**: Boxer tracks program completion, updates state

**Target Equivalent**:
- **Status**: MISSING
- **Location**: run_binary_executable() at shell_misc.cpp:583 has no callback after execution
- **Changes**: Program execution ends at line 667, no callback

**Compatibility**: MISSING

**Migration Strategy**:
1. Add callback after CALLBACK_RunRealInt(0x21) completes at line 663
2. Add callback after reg_sp restoration at line 666
3. Estimated effort: 5 hours

---

### INT-036: boxer_shellWillBeginBatchFile

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/shell/shell_misc.cpp:542`
- **Context**:
```cpp
if (strcasecmp(extension, ".bat") == 0)
{
    /* ... */
    boxer_shellWillBeginBatchFile(this, canonicalPath, args);  // <-- CALLBACK

    bf = std::make_shared<BatchFile>(this, fullname, name, line);
```
- **Purpose**: Notifies Boxer before batch file execution begins
- **Signature**: `void boxer_shellWillBeginBatchFile(DOS_Shell* shell, const char* path, const char* args)`
- **Usage**: Boxer tracks batch file lifecycle, prepares state

**Target Equivalent**:
- **Status**: MISSING
- **Location**: ExecuteProgram() handles .BAT files at shell_misc.cpp:505-523
- **Changes**: BatchFile pushed to stack (line 513-518) without callback

**Compatibility**: MISSING

**Migration Strategy**:
1. Add callback before batchfiles.emplace() at line 513
2. Pass resolved path and arguments
3. Estimated effort: 4 hours

---

### INT-037: boxer_shellDidEndBatchFile

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/shell/shell_batch.cpp:73`
- **Context**:
```cpp
BatchFile::~BatchFile() {
    delete cmd;
    assert(shell);
    shell->bf = prev;
    shell->echo = echo;

    boxer_shellDidEndBatchFile(shell, filename.c_str());  // <-- CALLBACK
}
```
- **Purpose**: Notifies Boxer when batch file execution completes (destructor)
- **Signature**: `void boxer_shellDidEndBatchFile(DOS_Shell* shell, const char* filename)`
- **Usage**: Boxer tracks batch completion, cleanup state

**Target Equivalent**:
- **Status**: MISSING
- **Location**: BatchFile has no destructor with callback
- **Changes**: Batch files stored in stack, automatically cleaned up via pop (shell.cpp:420)

**Compatibility**: MISSING

**Migration Strategy**:
1. Add destructor to BatchFile class with callback hook
2. Alternative: Add callback where batchfiles.pop() occurs (shell.cpp:420)
3. Need to preserve filename for callback
4. Estimated effort: 6 hours

---

### INT-038: boxer_shellShouldContinue

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/shell/shell.cpp:433,440`
- **Context**:
```cpp
// Location 1: After InputCommand
if (boxer_shellShouldContinue(this) && !boxer_hasPendingCommandsForShell(this))
{
    ParseLine(input_line);
    if (echo && !bf) WriteOut_NoParsing("\n");
}

// Location 2: Main loop condition
} while (boxer_shellShouldContinue(this) && !shutdown_requested);
```
- **Purpose**: Allows Boxer to interrupt shell execution loop (2 locations)
- **Signature**: `bool boxer_shellShouldContinue(DOS_Shell* shell)`
- **Usage**: Boxer can halt shell execution for cleanup or state changes

**Target Equivalent**:
- **Status**: MISSING (both locations)
- **Location**: Main loop at shell.cpp:480 uses only exit_cmd_called and DOSBOX_IsShutdownRequested()
- **Changes**: No external control over loop continuation

**Compatibility**: MISSING

**Migration Strategy**:
1. Add virtual method for continuation check
2. Insert checks in main loop at line 480
3. Insert check before ParseLine() at line 486
4. Estimated effort: 4 hours

---

## Summary Table

| ID | Name | Legacy Location | Target Status | Compatibility | Complexity | Notes |
|----|------|-----------------|---------------|---------------|------------|-------|
| INT-023 | shellWillStart | shell.cpp:342 | MISSING | MISSING | MEDIUM | 8 hours - Need shell tracking |
| INT-024 | shellDidFinish | shell.cpp:364,444 | MISSING | MISSING | MEDIUM | 6 hours - Multiple exit points |
| INT-025 | shellWillStartAutoexec | shell.cpp:371 | MISSING | MISSING | LOW | 4 hours - Simple insertion |
| INT-026 | didReturnToShell | shell.cpp:428 | MISSING | MISSING | LOW | 3 hours - Simple insertion |
| INT-027 | shellShouldRunCommand | shell_cmds.cpp:182 | MISSING | MISSING | MEDIUM | 6 hours - Command interception |
| INT-028 | shellWillReadCommandInputFromHandle | shell_misc.cpp:75 | MISSING | MISSING | HIGH | 8 hours - Complex input refactor |
| INT-029 | shellDidReadCommandInputFromHandle | shell_misc.cpp:82 | MISSING | MISSING | MEDIUM | 6 hours - Input handling |
| INT-030 | handleShellCommandInput | shell_misc.cpp:90 | MISSING | MISSING | HIGH | 12 hours - CommandPrompt integration |
| INT-031 | hasPendingCommandsForShell | shell.cpp:408 | MISSING | MISSING | HIGH | 10 hours - Command queue system |
| INT-032 | executeNextPendingCommandForShell | shell.cpp:410 | MISSING | MISSING | HIGH | Included in INT-031 |
| INT-033 | shellShouldDisplayStartupMessages | shell.cpp:377 | MISSING | MISSING | LOW | 2 hours - Simple check |
| INT-034 | shellWillExecuteFileAtDOSPath | shell_misc.cpp:557 | MISSING | MISSING | MEDIUM | 5 hours - Pre-execution hook |
| INT-035 | shellDidExecuteFileAtDOSPath | shell_misc.cpp:656 | MISSING | MISSING | MEDIUM | 5 hours - Post-execution hook |
| INT-036 | shellWillBeginBatchFile | shell_misc.cpp:542 | MISSING | MISSING | MEDIUM | 4 hours - Batch start hook |
| INT-037 | shellDidEndBatchFile | shell_batch.cpp:73 | MISSING | MISSING | MEDIUM | 6 hours - Batch end hook |
| INT-038 | shellShouldContinue | shell.cpp:433,440 | MISSING | MISSING | LOW | 4 hours - Loop control |

---

## Shell Functionality Compatibility

### Callbacks That Still Work
**NONE** - Zero integration points have direct equivalents in target

### Callbacks That Need Adaptation
**NONE** - All callbacks are completely missing, not just changed

### Callbacks That Are Missing
**ALL 15 INTEGRATION POINTS**

1. **Lifecycle (4 points)**: Complete shell lifecycle tracking is absent
2. **Command Processing (3 points)**: No command interception or injection
3. **Input Handling (3 points)**: No input interception or modification
4. **Command Queue (2 points)**: No external command queue system
5. **File Execution (3 points)**: No program/batch execution tracking

**Alternative Approaches Required**:
- Virtual method pattern for callbacks
- Observer/listener registration system
- Shell state tracking registry
- Command queue implementation
- Access to batch file stack

---

## Migration Complexity

### By Integration Point:

- **DROP-IN**: 0 points × 0.5 hours = 0 hours
- **MOVED**: 0 points × 1 hour = 0 hours
- **SIGNATURE**: 0 points × 2 hours = 0 hours
- **REFACTORED**: 0 points × 4 hours = 0 hours
- **MISSING**: 15 points = varies by complexity:
  - LOW (5 points): 3-4 hours each = 17 hours
  - MEDIUM (7 points): 5-6 hours each = 40 hours
  - HIGH (3 points): 8-12 hours each = 28 hours

**Subtotal**: 85 hours for direct callback implementation

**Additional Infrastructure**:
- Shell tracking registry: 8 hours
- Command queue system: 12 hours
- Callback registration framework: 10 hours
- Testing and integration: 15 hours

**Total Base Effort**: 130 hours

**Risk Buffer (30%)**: 39 hours

**Total Effort**: **120-160 hours**

---

## Risks

### HIGH Risks

1. **Complete Integration Absence**
   - **Impact**: All shell integration must be reimplemented from scratch
   - **Mitigation**: Design extension point architecture before implementation
   - **Effort**: 16+ hours for architecture design

2. **Private Batch File Stack**
   - **Impact**: Cannot access batch file state or inject callbacks
   - **Mitigation**: Expose batch file access or add notification system
   - **Effort**: 12 hours to add accessor methods

3. **CommandPrompt Encapsulation**
   - **Impact**: Input handling is fully encapsulated in CommandPrompt class
   - **Mitigation**: Refactor to allow external input hooks
   - **Effort**: 16 hours for refactoring

4. **No Shell Instance Tracking**
   - **Impact**: No currentShell global or equivalent
   - **Mitigation**: Add registry pattern or restore tracking
   - **Effort**: 8 hours for tracking system

### MEDIUM Risks

1. **String View Parameters**
   - **Impact**: Many methods use std::string_view instead of char*
   - **Mitigation**: Convert Boxer integration layer to C++ string types
   - **Effort**: 8 hours for conversion

2. **Stack-based Batch Files**
   - **Impact**: std::stack doesn't provide iteration or access
   - **Mitigation**: Use std::deque or add accessor methods
   - **Effort**: 6 hours

3. **Improved Redirection**
   - **Impact**: Redirection system completely rewritten
   - **Mitigation**: Ensure callbacks work with new redirection
   - **Effort**: 4 hours testing

---

## Recommendations

### 1. **Create Extension Point Architecture** (Priority: CRITICAL)
- Design virtual method system for all 15 integration points
- Use observer/listener pattern for lifecycle events
- Allow registration of external handlers
- **Effort**: 16 hours
- **Benefit**: Clean, maintainable integration layer

### 2. **Restore Shell State Access** (Priority: HIGH)
- Add shell instance tracking (registry or global)
- Expose batch file stack access (const accessor)
- Add command queue system to DOS_Shell
- **Effort**: 20 hours
- **Benefit**: Enables all integration points

### 3. **Refactor Input Handling** (Priority: HIGH)
- Add hooks to CommandPrompt class
- Allow external input interception
- Support command injection
- **Effort**: 16 hours
- **Benefit**: Restores INT-028, INT-029, INT-030

### 4. **Implement Callback Framework** (Priority: HIGH)
- Create callback registration system
- Add callbacks for program/batch execution
- Add callbacks for shell lifecycle
- **Effort**: 30 hours
- **Benefit**: Systematic callback support

### 5. **Incremental Migration Strategy** (Priority: CRITICAL)
Implement in phases:
- **Phase 1** (40 hours): Core infrastructure (shell tracking, callbacks)
- **Phase 2** (40 hours): Lifecycle and command hooks (INT-023-027)
- **Phase 3** (30 hours): Input handling hooks (INT-028-030)
- **Phase 4** (20 hours): Queue and display (INT-031-033)
- **Phase 5** (20 hours): Execution tracking (INT-034-038)

### 6. **Alternative: DOSBox Side Integration Module**
Instead of modifying DOSBox Staging shell directly, create a separate integration module:
- Register as DOS_Shell subclass
- Override virtual methods
- Implement all callbacks via overrides
- **Effort**: 80 hours (less than direct modification)
- **Benefit**: Cleaner separation, easier maintenance
- **Risk**: May not have access to required state

---

## Blockers/Open Questions

1. **Q**: Can we subclass DOS_Shell to add integration points?
   - **Status**: UNKNOWN - need to verify DOS_Shell is virtual
   - **Impact**: Could reduce migration effort by 30%

2. **Q**: Is there a DOSBox Staging extension API we should use?
   - **Status**: UNKNOWN - need to check documentation
   - **Impact**: May provide better integration path

3. **Q**: Can we access batch file stack indirectly?
   - **Status**: UNKNOWN - need to check if accessors exist
   - **Impact**: Required for INT-036, INT-037

4. **Q**: How should we handle the currentShell global removal?
   - **Options**:
     - Restore global (simple but regresses architecture)
     - Use registry pattern (clean but more complex)
     - Pass shell pointer through callbacks (requires refactoring)
   - **Status**: NEEDS DECISION

5. **Q**: Should we modify DOSBox Staging or create wrapper?
   - **Options**:
     - Direct modification: More invasive, harder to maintain
     - Wrapper/subclass: Cleaner, but may lack state access
   - **Status**: NEEDS DECISION

---

## Next Steps

1. **Review with Boxer team**: Confirm all 15 integration points are still needed
2. **Architectural decision**: Choose integration strategy (direct modification vs. subclass)
3. **Prototype**: Test subclassing DOS_Shell to verify feasibility
4. **Design review**: Create detailed extension point architecture
5. **Prioritize**: Determine which integration points are critical vs. nice-to-have
6. **Coordinate with Agent 1B.2**: Ensure DOS integration points are compatible

---

## Conclusion

All 15 shell integration points are completely MISSING from DOSBox Staging. The target has been significantly refactored with modern C++ patterns, improved architecture, and better separation of concerns, but lacks any extension mechanism for external integration. A complete reimplementation of the integration layer is required, estimated at 120-160 hours of effort.

The recommended approach is to design a clean extension point architecture using virtual methods and the observer pattern, then implement incrementally in 5 phases. An alternative approach using a DOS_Shell subclass may reduce effort but requires verification of feasibility.

**Risk Level**: HIGH
**Migration Effort**: 120-160 hours
**Critical Dependencies**: Shell instance tracking, batch file stack access, command queue system
**Blocker**: Complete absence of extension points
