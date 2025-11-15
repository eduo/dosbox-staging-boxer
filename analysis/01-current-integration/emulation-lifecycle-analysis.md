# Emulation Loop & Lifecycle Analysis

**Agent**: Agent 1B.8 - Emulation Loop & Lifecycle
**Created**: 2025-11-14T00:00:00Z
**Status**: Completed
**Dependencies**: Agent 1A

## Summary

The legacy DOSBox integration includes 5 critical lifecycle hooks that give Boxer control over emulation execution. The target DOSBox has a significantly simplified run loop with NO lifecycle callbacks, requiring all 5 integration points to be re-implemented. E_Exit error handling is compatible (minor signature changes). Localization system has been completely redesigned but can be adapted. **Migration effort: 16-20 hours** for core lifecycle hooks, with INT-059 (loop control) being the critical path.

## Legacy Emulation Architecture

### Main Loop Structure

**File**: `/home/user/dosbox-staging-boxer/src/dosbox.cpp`

**Normal_Loop** (lines 156-187):
```cpp
static Bitu Normal_Loop(void) {
    Bits ret;
    while (1) {
        //--Added 2009-12-27 by Alun Bestor to short-circuit the emulation loop
        if (!boxer_runLoopShouldContinue()) return 1;  // LINE 160 - INT-059
        //--End of modifications

        if (PIC_RunQueue()) {
            ret = (*cpudecoder)();
            if (GCC_UNLIKELY(ret<0)) return 1;
            if (ret>0) {
                if (GCC_UNLIKELY(ret >= CB_MAX)) return 0;
                Bitu blah = (*CallBack_Handlers[ret])();
                if (GCC_UNLIKELY(blah)) return blah;
            }
#if C_DEBUG
            if (DEBUG_ExitLoop()) return 0;
#endif
        } else {
            if (!GFX_Events()) {
                return 0;
            }
            //--Check again at this point in case our own events have cancelled the emulation.
            if (!boxer_runLoopShouldContinue()) return 1;  // LINE 179 - INT-059
            //--End of modifications
            if (ticksRemain > 0) {
                TIMER_AddTick();
                ticksRemain--;
            } else {increaseticks();return 0;}
        }
    }
}
```

**DOSBOX_RunMachine** (lines 345-358):
```cpp
void DOSBOX_RunMachine()
{
    Bitu ret = 0;
    while (ret == 0 && !shutdown_requested) {
        //--Modified 2011-09-25 by Alun Bestor to bracket iterations of the run loop
        //with our own callbacks. We pass along the contextInfo parameter so that
        //Boxer knows which iteration of the runloop is running (in case of nested runloops).
        void *contextInfo;
        boxer_runLoopWillStartWithContextInfo(&contextInfo);  // LINE 353 - INT-057
        ret=(*loop)();
        boxer_runLoopDidFinishWithContextInfo(contextInfo);   // LINE 355 - INT-058
        //--End of modifications.
    };
}
```

### Lifecycle Callbacks

**INT-059: boxer_runLoopShouldContinue** (CRITICAL)
- **Location**: dosbox.cpp:160, 179
- **Purpose**: Allows Boxer to abort emulation at any time (e.g., when window closes, user cancels)
- **Call Frequency**: Every iteration of the main loop (thousands of times per second)
- **Return**: false = stop emulation immediately
- **Context**: Called at two strategic points:
  1. Line 160: Beginning of loop iteration (before CPU execution)
  2. Line 179: After GFX events processed (before timer tick)

**INT-057: boxer_runLoopWillStartWithContextInfo** (CRITICAL)
- **Location**: dosbox.cpp:353
- **Purpose**: Pre-loop iteration callback, allows Boxer to prepare for emulation cycle
- **Parameters**: `void **contextInfo` - output parameter for context tracking
- **Usage**: Boxer allocates/initializes context for this loop iteration
- **Context**: Called before each `(*loop)()` execution

**INT-058: boxer_runLoopDidFinishWithContextInfo** (CRITICAL)
- **Location**: dosbox.cpp:355
- **Purpose**: Post-loop iteration callback, cleanup and state tracking
- **Parameters**: `void *contextInfo` - context from INT-057
- **Usage**: Boxer deallocates/processes context after loop iteration
- **Context**: Called after each `(*loop)()` execution

### Error Handling

**INT-013: E_Exit / boxer_die**

**Declaration**: `/home/user/dosbox-staging-boxer/include/dosbox.h:38-39`
```cpp
//[[noreturn]] void E_Exit(const char *message, ...)
//        GCC_ATTRIBUTE(__format__(__printf__, 1, 2));
```

**Implementation**: `/home/user/dosbox-staging-boxer/src/misc/support.cpp:306-317` (commented out)
```cpp
/*
void E_Exit(const char *format, ...)
{
#if C_DEBUG && C_HEAVY_DEBUG
    DEBUG_HeavyWriteLogInstruction();
#endif
    va_list msg;
    va_start(msg, format);
    vsnprintf(e_exit_buf, ARRAY_LEN(e_exit_buf), format, msg);
    va_end(msg);
    ABORT_F("%s", e_exit_buf);
}
*/
```

**Note**: E_Exit is commented out in legacy but expected to be provided by Boxer via BXCoalface.h (which doesn't exist in this repo). The macro likely redirects E_Exit calls to `boxer_die()`.

**Usage Example**: dosbox.cpp:432
```cpp
E_Exit("DOSBOX:Unknown machine type %s", mtype.c_str());
```

### Exit Mechanism

**Graceful Shutdown**: `shutdown_requested` boolean flag
- Set to true to break out of `DOSBOX_RunMachine` loop
- Allows emulation to finish current iteration cleanly

**Immediate Termination**: `E_Exit()` / `ABORT_F()`
- Throws exception or calls abort
- Used for unrecoverable errors

### Localization

**INT-081: boxer_localizedStringForKey**

**Location**: `/home/user/dosbox-staging-boxer/src/misc/messages.cpp:125-128`
```cpp
const char * MSG_Get(char const * msg)
{
    return boxer_localizedStringForKey(msg);
}
```

**Purpose**: Routes all DOSBox message lookups through Boxer's localization system
- Replaces the standard MSG_Get implementation
- Allows Boxer to provide its own translations
- Called for all user-visible messages

## Target Emulation Architecture

### Main Loop Structure

**File**: `/home/user/dosbox-staging/src/dosbox.cpp`

**normal_loop** (lines 107-163):
```cpp
static Bitu normal_loop()
{
    Bits ret;

    while (true) {
        if (PIC_RunQueue()) {
            ret = (*cpudecoder)();
            if (ret < 0) {
                return 1;
            }
            if (ret > 0) {
                if (ret >= CB_MAX) {
                    return 0;
                }
                Bitu result = (*Callback_Handlers[ret])();
                if (result) {
                    return result;
                }
            }
#if C_DEBUGGER
            if (DEBUG_ExitLoop()) {
                return 0;
            }
#endif
        } else {
            // Host-rate presentation mode handling
            if (GFX_GetPresentationMode() == PresentationMode::HostRate) {
                GFX_MaybePresentFrame();
            }

            if (!GFX_PollAndHandleEvents()) {
                return 0;
            }
            if (ticks.remain > 0) {
                TIMER_AddTick();
                --ticks.remain;
            } else {
                increase_ticks();
                return 0;
            }
        }
    }
}
```

**DOSBOX_RunMachine** (lines 413-417):
```cpp
void DOSBOX_RunMachine()
{
    while ((*loop)() == 0 && !is_shutdown_requested)
        ;
}
```

**KEY DIFFERENCES**:
1. **NO lifecycle callbacks** - no INT-057, INT-058, INT-059 equivalent
2. Much cleaner, simpler structure
3. Uses `is_shutdown_requested` instead of `shutdown_requested`
4. Host-rate presentation mode added (GFX_MaybePresentFrame)
5. Modern C++ style (true/false, structured ticks)

### Hook Points

To re-implement Boxer's lifecycle control, we need to add hooks at:

**For INT-059 (runLoopShouldContinue)**:
- **Primary Location**: Line 111 (start of `while (true)` loop)
- **Secondary Location**: Line 151 (after `GFX_PollAndHandleEvents()`)
- **Implementation**: Add `if (!boxer_runLoopShouldContinue()) return 1;`

**For INT-057/058 (runLoopWill/DidFinish)**:
- **Location**: Lines 414-416 (inside `DOSBOX_RunMachine` while loop)
- **Implementation**: Wrap `(*loop)()` call with callbacks:
```cpp
void DOSBOX_RunMachine()
{
    while (!is_shutdown_requested) {
        void *contextInfo;
        boxer_runLoopWillStartWithContextInfo(&contextInfo);
        auto ret = (*loop)();
        boxer_runLoopDidFinishWithContextInfo(contextInfo);
        if (ret != 0) break;
    }
}
```

### Error Handling

**E_Exit** - FULLY COMPATIBLE

**Declaration**: `/home/user/dosbox-staging/src/dosbox.h:47-50`
```cpp
// The E_Exit function throws an exception to quit. Call it in unexpected
// circumstances.
[[noreturn]] void E_Exit(const char *message, ...)
        GCC_ATTRIBUTE(__format__(__printf__, 1, 2));
```

**Implementation**: `/home/user/dosbox-staging/src/misc/support.cpp:149-159`
```cpp
void E_Exit(const char* format, ...)
{
#if C_DEBUGGER && C_HEAVY_DEBUGGER
    DEBUG_HeavyWriteLogInstruction();
#endif
    va_list msg;
    va_start(msg, format);
    vsnprintf(e_exit_buf, ARRAY_LEN(e_exit_buf), format, msg);
    va_end(msg);
    ABORT_F("%s", e_exit_buf);
}
```

**Changes from Legacy**:
- Added `[[noreturn]]` attribute
- Parameter renamed `message` → `format`
- Uses `const char*` instead of `char const *`
- Implementation is essentially identical

## Integration Point Analysis

### INT-013: boxer_die (E_Exit)

**Legacy Implementation**:
- **Location**: E_Exit calls throughout codebase (commented out in support.cpp)
- **Purpose**: Terminate emulation with error message
- **Signature**: `void E_Exit(const char *message, ...)`
- **Mechanism**: Formats error message, calls ABORT_F (likely redirected to boxer_die)

**Target Equivalent**:
- **Status**: EXISTS (fully implemented)
- **Location**: `/home/user/dosbox-staging/src/misc/support.cpp:149-159`
- **Changes**:
  - Signature: `[[noreturn]] void E_Exit(const char* format, ...)`
  - Parameter name changed: `message` → `format`
  - Modern C++ attributes added

**Compatibility**: SIGNATURE CHANGE (minor)

**Migration Strategy**:
1. **Option A (Recommended)**: Keep E_Exit as-is, add `#define boxer_die E_Exit` in Boxer integration header
2. **Option B**: Wrap E_Exit to provide boxer_die function that calls E_Exit
3. **Option C**: Replace E_Exit body with call to Boxer's error handler

**Recommendation**: Use Option A - no modification to target DOSBox needed, Boxer just maps boxer_die → E_Exit.

---

### INT-057: boxer_runLoopWillStartWithContextInfo

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/dosbox.cpp:353`
- **Purpose**: Pre-loop initialization callback before each loop iteration
- **Context**: Called inside `DOSBOX_RunMachine` before `(*loop)()`
- **Signature**: `void boxer_runLoopWillStartWithContextInfo(void **contextInfo)`
- **Usage**: Boxer allocates context, stores pointer in `*contextInfo`

**Target Equivalent**:
- **Status**: MISSING (no equivalent exists)
- **Location**: Would need to be added to `/home/user/dosbox-staging/src/dosbox.cpp:415` (inside DOSBOX_RunMachine)
- **Changes**: Target DOSBOX_RunMachine is single-line, needs to be expanded

**Migration Strategy**:
1. Expand `DOSBOX_RunMachine` to multi-line loop body
2. Add `void *contextInfo;` declaration
3. Call `boxer_runLoopWillStartWithContextInfo(&contextInfo);` before `(*loop)()`
4. Declare extern function in Boxer integration header

**Implementation**:
```cpp
void DOSBOX_RunMachine()
{
    while (!is_shutdown_requested) {
        void *contextInfo = nullptr;
        boxer_runLoopWillStartWithContextInfo(&contextInfo);

        auto ret = (*loop)();

        boxer_runLoopDidFinishWithContextInfo(contextInfo);

        if (ret != 0) break;
    }
}
```

---

### INT-058: boxer_runLoopDidFinishWithContextInfo

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/dosbox.cpp:355`
- **Purpose**: Post-loop cleanup callback after each loop iteration
- **Context**: Called inside `DOSBOX_RunMachine` after `(*loop)()`
- **Signature**: `void boxer_runLoopDidFinishWithContextInfo(void *contextInfo)`
- **Usage**: Boxer processes/deallocates context from INT-057

**Target Equivalent**:
- **Status**: MISSING (no equivalent exists)
- **Location**: Would need to be added to `/home/user/dosbox-staging/src/dosbox.cpp:415` (inside DOSBOX_RunMachine)
- **Changes**: Same modification as INT-057

**Migration Strategy**:
Same as INT-057 - both callbacks are added in the same location. See implementation above.

---

### INT-059: boxer_runLoopShouldContinue

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/dosbox.cpp:160, 179`
- **Purpose**: Abort emulation immediately if Boxer needs to stop (window close, user cancel, etc.)
- **Context**: Called twice per loop iteration:
  1. Line 160: Start of iteration (before CPU execution)
  2. Line 179: After GFX events (before timer tick)
- **Signature**: `bool boxer_runLoopShouldContinue(void)`
- **Return**: `false` = stop emulation immediately

**Target Equivalent**:
- **Status**: MISSING (no equivalent exists)
- **Location**: Would need to be added to `/home/user/dosbox-staging/src/dosbox.cpp`:
  - Primary: Line 111 (start of while loop in normal_loop)
  - Secondary: Line 151 (after GFX_PollAndHandleEvents)
- **Changes**: Requires modification to normal_loop function

**Compatibility**: MISSING - CRITICAL FUNCTIONALITY

**Migration Strategy**:
1. Add external function declaration in Boxer integration header
2. Modify `normal_loop()` at two locations:

**Location 1** (start of loop):
```cpp
static Bitu normal_loop()
{
    Bits ret;

    while (true) {
        // BOXER INTEGRATION: Allow immediate abort
        if (!boxer_runLoopShouldContinue()) return 1;

        if (PIC_RunQueue()) {
            // ... rest of code
```

**Location 2** (after GFX events):
```cpp
        } else {
            if (GFX_GetPresentationMode() == PresentationMode::HostRate) {
                GFX_MaybePresentFrame();
            }

            if (!GFX_PollAndHandleEvents()) {
                return 0;
            }

            // BOXER INTEGRATION: Check again after events
            if (!boxer_runLoopShouldContinue()) return 1;

            if (ticks.remain > 0) {
```

**CRITICAL**: This is the MOST IMPORTANT integration point. Without it, Boxer cannot:
- Respond to window close events during emulation
- Cancel long-running operations
- Switch between programs gracefully
- Handle errors in Objective-C layer

---

### INT-081: boxer_localizedStringForKey

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/misc/messages.cpp:125-128`
- **Purpose**: Route all message lookups through Boxer's localization
- **Signature**: `const char * boxer_localizedStringForKey(const char *key)`
- **Usage**: Replaces entire `MSG_Get` implementation

**Target Equivalent**:
- **Status**: Different approach - more sophisticated system
- **Location**: `/home/user/dosbox-staging/src/misc/messages.cpp` (full implementation)
- **Changes**:
  - Target has UTF-8 → code page conversion
  - ANSI markup tag processing
  - Translation file loading (.po format)
  - Fuzzy/invalid message tracking

**Target System** (`/home/user/dosbox-staging/src/misc/messages.h`):
```cpp
// Get translated message for DOS console
std::string MSG_Get(const std::string& message_key);

// Get original English in UTF-8
std::string MSG_GetEnglishRaw(const std::string& message_key);

// Get translated in UTF-8
std::string MSG_GetTranslatedRaw(const std::string& message_key);
```

**Compatibility**: BEHAVIOR DIFFERENT - can be adapted

**Migration Strategy** (3 options):

**Option A: Replace MSG_Get** (Simplest, loses target features):
```cpp
std::string MSG_Get(const std::string& message_key)
{
    const char *result = boxer_localizedStringForKey(message_key.c_str());
    return result ? std::string(result) : " MESSAGE NOT FOUND! ";
}
```

**Option B: Hybrid approach** (Recommended):
```cpp
std::string MSG_Get(const std::string& message_key)
{
    // Try Boxer localization first
    const char *boxer_result = boxer_localizedStringForKey(message_key.c_str());
    if (boxer_result && boxer_result[0] != '\0') {
        return std::string(boxer_result);
    }

    // Fall back to target's localization system
    return MSG_GetOriginal(message_key);  // rename original implementation
}
```

**Option C: No override** (Keep target system):
- Don't override MSG_Get at all
- Use target's localization system
- Boxer provides .po translation files
- Most compatible but requires Boxer to adapt

**Recommendation**: Use Option B for backward compatibility with existing Boxer translations while gaining target's UTF-8 and code page features.

---

## Summary Table

| ID | Name | Criticality | Target Status | Complexity | Hours |
|----|------|-------------|---------------|------------|-------|
| INT-013 | boxer_die (E_Exit) | CORE | EXISTS | LOW | 1 |
| INT-057 | runLoopWillStart | CORE | MISSING | MEDIUM | 4 |
| INT-058 | runLoopDidFinish | CORE | MISSING | MEDIUM | 4 |
| INT-059 | runLoopShouldContinue | CORE | MISSING | HIGH | 8 |
| INT-081 | localizedStringForKey | Minor | DIFFERENT | MEDIUM | 3 |

**TOTAL ESTIMATED EFFORT**: 20 hours

## Lifecycle Control Compatibility

### Loop Control
**Status**: Needs complete reimplementation

**Hook Points**:
1. **normal_loop** (dosbox.cpp:111, 151): Add INT-059 checks
2. **DOSBOX_RunMachine** (dosbox.cpp:414-416): Add INT-057/058 wrapping

**Migration Approach**:
```cpp
// In dosbox.cpp

// Hook point 1: normal_loop start
static Bitu normal_loop()
{
    Bits ret;
    while (true) {
        #ifdef BOXER_INTEGRATION
        if (!boxer_runLoopShouldContinue()) return 1;
        #endif

        // ... existing code ...
    }
}

// Hook point 2: DOSBOX_RunMachine
void DOSBOX_RunMachine()
{
    #ifdef BOXER_INTEGRATION
    while (!is_shutdown_requested) {
        void *contextInfo = nullptr;
        boxer_runLoopWillStartWithContextInfo(&contextInfo);
        auto ret = (*loop)();
        boxer_runLoopDidFinishWithContextInfo(contextInfo);
        if (ret != 0) break;
    }
    #else
    while ((*loop)() == 0 && !is_shutdown_requested)
        ;
    #endif
}
```

### Error Handling
**Status**: Compatible with minor signature adaptation

**E_Exit Equivalent**: Direct usage, no wrapper needed

**Migration**: Add to Boxer integration header:
```cpp
// BXCoalface.h or boxer_integration.h
#define boxer_die E_Exit
```

### Localization
**Status**: Target system more advanced, can be overridden

**Migration**:
- **Short term**: Override MSG_Get to call boxer_localizedStringForKey
- **Long term**: Migrate Boxer's translations to .po format and use target system

## Migration Complexity

**Total Effort**: 16-20 hours

**Breakdown**:

### INT-059 (loop control): MUST work - 8 hours
- **Priority**: CRITICAL (P0)
- **Complexity**: HIGH
- **Tasks**:
  - Add checks to normal_loop (2 locations): 2 hours
  - Test emulation abort scenarios: 2 hours
  - Handle nested loop cases: 2 hours
  - Edge case testing (mid-frame abort, etc.): 2 hours

### INT-057/058 (lifecycle): MUST work - 8 hours
- **Priority**: CRITICAL (P0)
- **Complexity**: MEDIUM
- **Tasks**:
  - Modify DOSBOX_RunMachine: 2 hours
  - Implement context tracking on Boxer side: 3 hours
  - Test nested runloop scenarios: 2 hours
  - Memory leak testing: 1 hour

### INT-013 (error handling): MUST work - 1 hour
- **Priority**: CRITICAL (P0)
- **Complexity**: LOW
- **Tasks**:
  - Add macro definition: 0.5 hours
  - Test error scenarios: 0.5 hours

### INT-081 (localization): Nice to have - 3 hours
- **Priority**: LOW (P2)
- **Complexity**: MEDIUM
- **Tasks**:
  - Override MSG_Get: 1 hour
  - Test all message keys: 1 hour
  - UTF-8/code page compatibility testing: 1 hour

## Risks

### CRITICAL Risks (Would Block Entire Upgrade)

**RISK-LC-001: INT-059 cannot be implemented**
- **Impact**: Boxer cannot abort emulation, window close hangs
- **Likelihood**: LOW (straightforward implementation)
- **Mitigation**: Two hook points provide redundancy; normal_loop is well-defined

**RISK-LC-002: Context tracking breaks nested loops**
- **Impact**: Crashes when programs call other programs
- **Likelihood**: MEDIUM (nested loops are complex)
- **Mitigation**: Maintain context stack on Boxer side, test with CALL scenarios

**RISK-LC-003: Performance degradation from callback overhead**
- **Impact**: Emulation slowdown from extra function calls
- **Likelihood**: MEDIUM (callbacks in hot path)
- **Mitigation**:
  - Use inline functions for boxer_runLoopShouldContinue
  - Profile before/after integration
  - Consider batch checking (every N iterations)

### MEDIUM Risks (Significant Impact)

**RISK-LC-004: Target loop structure changes in future**
- **Impact**: Hooks break after upstream merge
- **Likelihood**: MEDIUM
- **Mitigation**:
  - Use #ifdef BOXER_INTEGRATION to isolate changes
  - Document hook locations clearly
  - Create automated tests for hook behavior

**RISK-LC-005: Shutdown flag race conditions**
- **Impact**: Emulation doesn't stop cleanly
- **Likelihood**: LOW
- **Mitigation**: Use atomic operations for is_shutdown_requested

**RISK-LC-006: Localization encoding mismatches**
- **Impact**: Garbled text in some languages
- **Likelihood**: MEDIUM (if using Option A for INT-081)
- **Mitigation**: Use Option B (hybrid) or Option C (pure target system)

### LOW Risks (Minor Impact)

**RISK-LC-007: Context info memory leaks**
- **Impact**: Memory grows over time
- **Likelihood**: LOW (if INT-058 always called)
- **Mitigation**: Add assertions, test with valgrind

**RISK-LC-008: E_Exit message format changes**
- **Impact**: Error messages formatted differently
- **Likelihood**: LOW
- **Mitigation**: Test all E_Exit call sites

## Recommendations

### Priority Actions for Core Integration Points

1. **[P0] Implement INT-059 FIRST** (8 hours)
   - This is the absolute minimum for Boxer to function
   - Without it, emulation cannot be controlled
   - Add hooks to normal_loop at two locations
   - Test thoroughly with window close, user cancel, etc.

2. **[P0] Implement INT-057/058** (8 hours)
   - Required for nested loop handling
   - Modify DOSBOX_RunMachine
   - Test with programs that call other programs (COMMAND.COM scenarios)

3. **[P0] Map INT-013 to E_Exit** (1 hour)
   - Simple macro definition
   - Test error handling paths

4. **[P2] Decide on localization strategy** (3 hours)
   - Recommend Option B (hybrid) for backward compatibility
   - Can defer to later phase if needed

### Testing Strategy for Emulation Loop

**Unit Tests**:
- Test boxer_runLoopShouldContinue returning false at different points
- Test context allocation/deallocation in INT-057/058
- Test nested loop scenarios

**Integration Tests**:
1. **Window Close Test**: Close Boxer window during emulation, verify clean shutdown
2. **User Cancel Test**: Cancel operation mid-execution
3. **Nested Programs Test**: Run program that calls another program (CALL, TSR)
4. **Long-Running Test**: Run for hours, check for memory leaks
5. **Error Handling Test**: Trigger E_Exit, verify Boxer error dialog

**Performance Tests**:
- Measure FPS before/after integration
- Profile callback overhead
- Test with high cycles (max performance)
- Test with low cycles (precise timing)

### Fallback Approaches if Standard Hooks Don't Work

**If INT-059 cannot be added to normal_loop**:
- **Alternative 1**: Modify GFX_PollAndHandleEvents to check Boxer state
- **Alternative 2**: Use signal/interrupt mechanism (SIGINT handler)
- **Alternative 3**: Modify PIC_RunQueue to check Boxer state (less desirable)

**If INT-057/058 context tracking breaks**:
- **Alternative 1**: Use global context pointer instead of pass-through
- **Alternative 2**: Move context tracking entirely to Boxer side
- **Alternative 3**: Simplify to single context (no nesting support)

**If E_Exit signature incompatible**:
- **Alternative 1**: Create boxer_die wrapper that calls E_Exit
- **Alternative 2**: Modify E_Exit to call boxer_die internally
- **Alternative 3**: Replace E_Exit implementation entirely

## Blockers/Open Questions

### Critical Issues

**BLOCKER-001: Are there other loop types besides Normal_Loop?**
- **Question**: Does target have other loop handlers that also need hooks?
- **Impact**: If yes, need to add hooks to all loop types
- **Investigation**: Search for other LoopHandler implementations
- **Resolution**: Check DOSBOX_SetLoop calls, identify all loop types

**BLOCKER-002: How does target handle SDL event processing?**
- **Question**: Does GFX_PollAndHandleEvents interfere with Boxer's event handling?
- **Impact**: Potential event loop conflicts
- **Investigation**: Trace SDL event flow in target
- **Resolution**: Ensure Boxer's NSApplication runloop cooperates with SDL

**BLOCKER-003: Thread safety of lifecycle callbacks**
- **Question**: Are callbacks always called from main thread?
- **Impact**: If multi-threaded, need synchronization
- **Investigation**: Check if emulation runs on separate thread
- **Resolution**: Document threading model, add locks if needed

### Open Questions (Non-Blocking)

**QUESTION-001: Can we use lambda captures instead of void* context?**
- **Impact**: Cleaner C++ code
- **Investigation**: Test if Boxer's Objective-C can work with C++11 lambdas
- **Priority**: LOW (nice-to-have)

**QUESTION-002: Should we batch INT-059 checks to reduce overhead?**
- **Impact**: Better performance but less responsive
- **Investigation**: Profile call frequency, measure overhead
- **Priority**: MEDIUM (optimization)

**QUESTION-003: Can we use target's localization for new messages?**
- **Impact**: Consistent localization system
- **Investigation**: Assess effort to migrate Boxer's translations
- **Priority**: LOW (future enhancement)

## Next Steps

1. **Verify loop types**: Confirm target only uses normal_loop
2. **Prototype INT-059**: Add hooks and test basic functionality
3. **Thread safety audit**: Ensure all callbacks are main-thread safe
4. **Coordinate with Agent 1B.9**: Ensure SDL integration works with lifecycle hooks
5. **Create test plan**: Define comprehensive test scenarios for all 5 integration points
