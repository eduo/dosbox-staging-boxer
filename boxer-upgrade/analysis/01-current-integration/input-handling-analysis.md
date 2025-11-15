# Input Handling Integration Analysis

**Agent**: Agent 1B.5 - Input Handling
**Created**: 2025-11-14
**Status**: Completed
**Dependencies**: Agent 1A (Integration Point Discovery)

## Summary

The target DOSBox Staging has completely refactored the input handling system, removing all Boxer-specific API hooks. All 15 input integration points (joystick activation, keyboard buffer queries, keyboard layout management, lock key synchronization, input continuation, and paste buffer) are MISSING from the target. The target implements a modernized keyboard controller emulation with Intel 8042 microcontroller support but provides no external hooks for Boxer integration. **High-complexity migration required** (estimated 40-60 hours) to implement alternative integration strategies.

## Legacy Input Architecture

### Overview

The legacy DOSBox implements Boxer integration through direct API hooks imported via `BXCoalface.h`. The input system is split across four main components:

1. **Hardware Keyboard** (`keyboard.cpp`): Scancode buffer management, key repeat
2. **Hardware Joystick** (`joystick.cpp`): Gameport emulation with timing modes
3. **BIOS Keyboard** (`bios_keyboard.cpp`): INT 16h handler, lock key state, paste buffer
4. **DOS Keyboard Layout** (`dos_keyboard_layout.cpp`): International keyboard support

### Keyboard Layout System

The legacy system provides extensive keyboard layout management:

- **File-based layouts**: Loads .kl files or .sys keyboard libraries
- **Multi-layout support**: keyboard.sys, keybrd2.sys, keybrd3.sys with embedded layouts
- **Code page integration**: Couples keyboard layouts with DOS code pages
- **Submapping system**: Multiple layouts per file for different code pages
- **Diacritics support**: Complex accent character composition
- **Runtime switching**: Can toggle between US and foreign layouts

**Boxer Integration Points**:
- `boxer_keyboardLayoutName()` - Returns currently loaded layout name
- `boxer_keyboardLayoutLoaded()` - Checks if non-US layout is loaded
- `boxer_keyboardLayoutSupported()` - Checks if layout can be loaded without code page change
- `boxer_keyboardLayoutActive()` - Checks if foreign layout is currently active
- `boxer_setKeyboardLayoutActive()` - Activates/deactivates foreign layout
- `boxer_preferredKeyboardLayout()` - Provides auto-detection hint

### Lock Key Synchronization

Legacy system synchronizes lock key states between macOS and DOS:

**Implementation** (`bios_keyboard.cpp`):
- Line 311: `boxer_setCapsLockActive(flags1 & 0x40)` - Called when Caps Lock toggled
- Line 345: `boxer_setNumLockActive(flags1 & 0x20)` - Called when Num Lock toggled
- Line 350: `boxer_setScrollLockActive(flags1 & 0x10)` - Called when Scroll Lock toggled

**Mechanism**:
- DOS IRQ1 handler detects lock key press/release
- Immediately calls Boxer to update macOS LED state
- Maintains synchronization between host and guest

### Paste Buffer

Legacy implements clipboard paste via virtual keyboard buffer:

**Implementation** (`bios_keyboard.cpp`):
- Line 162, 188: `boxer_getNextKeyCodeInPasteBuffer(&code, consume)` - Retrieves pasted keycodes
- Integrated into `get_key()` and `check_key()` functions
- Paste buffer takes priority over DOS keyboard buffer

**Mechanism** (`dev_con.h` - NOT FOUND in target):
- Line 414: `boxer_numKeyCodesInPasteBuffer()` - Query remaining paste keys
- Boxer converts clipboard text to DOS keycodes
- DOSBox consumes keycodes as if typed

### Joystick Activation Control

Legacy allows Boxer to detect joystick usage:

**Implementation** (`joystick.cpp`):
- Lines 269, 275: `boxer_setJoystickActive(true)` - Called on gameport I/O access
- Enables Boxer to show/hide joystick configuration UI
- No joystick state queries, only activation notification

### Keyboard Buffer Query

Legacy exposes keyboard buffer space:

**Implementation** (`keyboard.cpp`):
- Line 77-81: `boxer_keyboardBufferRemaining()` - Returns free space in scancode buffer
- Buffer size: 32 scancodes (KEYBUFSIZE)
- Returns 0 if buffer full, otherwise space remaining

### Input Continuation Control

Legacy allows Boxer to interrupt keyboard polling:

**Implementation** (`bios_keyboard.cpp`):
- Line 488: `boxer_continueListeningForKeyEvents()` - Checked during INT 16h polling
- Returns CBRET_STOP if Boxer wants to interrupt
- Prevents DOS from hanging in keyboard wait loops

## Target Input Architecture

### System Changes

Target DOSBox Staging has fundamentally redesigned input handling:

**Keyboard Controller Emulation**:
- **Intel 8042 microcontroller emulation** (`intel8042.h`, keyboard.cpp)
- **Scancode set support**: Set 1 (always), Set 2 (optional), Set 3 (optional)
- **PS/2 keyboard protocol**: Full command set implementation
- **LED state management**: Tracks Scroll/Num/Caps Lock state
- **Typematic control**: Configurable key repeat rate and delay
- **Buffer overflow handling**: Sophisticated queue management

**Joystick Modernization**:
- **Calibration system**: Runtime X/Y axis calibration with hotkeys
- **Visibility control**: `JOY_ONLY_FOR_MAPPING` mode (hidden from DOS)
- **Axis rate constants**: Tunable timing parameters (replaced hardcoded values)
- **Type detection**: More granular joystick type configuration

**Keyboard Layout Refactoring**:
- **Resource bundling**: Keyboard layouts from `freedos-keyboard` resources
- **Result enumeration**: `KeyboardLayoutResult` enum for error handling
- **Code page separation**: Clear separation of layout and font loading
- **Locale integration**: Ties into broader DOS locale system (`dos_locale.h`)

### File Structure

**Target files present**:
- `/home/user/dosbox-staging/src/hardware/input/keyboard.cpp` - PS/2 keyboard emulation
- `/home/user/dosbox-staging/src/hardware/input/keyboard.h` - Public keyboard API
- `/home/user/dosbox-staging/src/hardware/input/joystick.cpp` - Gameport emulation
- `/home/user/dosbox-staging/src/hardware/input/joystick.h` - Public joystick API
- `/home/user/dosbox-staging/src/ints/bios_keyboard.cpp` - BIOS keyboard services
- `/home/user/dosbox-staging/src/dos/dos_keyboard_layout.cpp` - Layout management
- `/home/user/dosbox-staging/src/dos/dos_keyboard_layout.h` - Layout API
- `/home/user/dosbox-staging/src/hardware/input/private/intel8042.h` - Controller emulation
- `/home/user/dosbox-staging/src/hardware/input/private/intel8255.h` - PPI emulation

**Legacy files MISSING**:
- `include/dev_con.h` - NOT FOUND (paste buffer interface)
- `BXCoalface.h` import - NOT FOUND (all Boxer hooks removed)

**Architecture differences**:
- Keyboard moved from `src/hardware/` to `src/hardware/input/` subdirectory
- Private implementation headers in `private/` subdirectory
- No external integration hooks in public APIs
- Modern C++ (SPDX headers, unique_ptr, enums, constexpr)

## Integration Point Analysis

### INT-060: boxer_setJoystickActive

**Legacy Implementation**:
- **Location**: `joystick.cpp:269,275`
- **Purpose**: Notify Boxer when joystick is accessed
- **Signature**: `void boxer_setJoystickActive(bool active)`
- **Called from**: `read_p201_switchable()`, `write_p201_switchable()`
- **Behavior**: Called with `true` whenever gameport (I/O port 0x201) is read or written

**Target Equivalent**:
- **Status**: MISSING
- **Location**: NOT FOUND
- **Changes**: No equivalent functionality; joystick system completely refactored

**Compatibility**: MISSING

**Migration Strategy**:
1. **No direct replacement available**
2. **Option A - I/O Port Hooks**: Intercept gameport I/O in Boxer wrapper layer
3. **Option B - Joystick Enable Events**: Track `JOYSTICK_Enable()` calls
4. **Option C - Polling**: Periodically check `JOYSTICK_IsAccessible()`
5. **Recommendation**: Option A provides closest match to legacy behavior

---

### INT-062: boxer_keyboardBufferRemaining

**Legacy Implementation**:
- **Location**: `keyboard.cpp:77-81`
- **Purpose**: Query free space in keyboard scancode buffer
- **Signature**: `Bitu boxer_keyboardBufferRemaining()`
- **Returns**: Number of free slots (0 to KEYBUFSIZE=32)
- **Behavior**: Returns `KEYBUFSIZE - keyb.used` or 0 if buffer full

**Target Equivalent**:
- **Status**: MISSING
- **Location**: NOT FOUND
- **Changes**: Keyboard buffer internals are private; no query API exposed

**Compatibility**: MISSING

**Migration Strategy**:
1. **No direct replacement available**
2. **Buffer internals are private** in target (buffer[] is static local variable)
3. **Option A - Remove dependency**: Boxer may not need this information
4. **Option B - Add query API**: Patch target to expose buffer state
5. **Option C - Heuristic**: Assume buffer usually has space
6. **Recommendation**: Option A (review Boxer usage first); fallback to Option B if needed

---

### INT-063: boxer_keyboardLayoutLoaded

**Legacy Implementation**:
- **Location**: `dos_keyboard_layout.cpp:1152-1155`
- **Purpose**: Check if any keyboard layout is loaded
- **Signature**: `bool boxer_keyboardLayoutLoaded()`
- **Returns**: `true` if `loaded_layout != NULL`
- **Behavior**: Simple null pointer check

**Target Equivalent**:
- **Status**: REFACTORED
- **Location**: `dos_keyboard_layout.h:52` (public API)
- **Function**: `std::string DOS_GetLoadedLayout()`
- **Returns**: Empty string if no layout loaded, layout name otherwise

**Compatibility**: REFACTORED

**Migration Strategy**:
1. **Direct replacement available**: `!DOS_GetLoadedLayout().empty()`
2. **Minor signature change**: Returns string instead of bool
3. **Equivalent semantics**: Empty string = no layout loaded
4. **Effort**: LOW (1-2 hours) - Simple wrapper function

---

### INT-064: boxer_keyboardLayoutName

**Legacy Implementation**:
- **Location**: `dos_keyboard_layout.cpp:1145-1151`
- **Purpose**: Get the real layout name (not the active state)
- **Signature**: `const char* boxer_keyboardLayoutName()`
- **Returns**: Pointer to `current_keyboard_file_name` or NULL
- **Behavior**: Returns actual layout name, regardless of US/foreign toggle

**Target Equivalent**:
- **Status**: REFACTORED
- **Location**: `dos_keyboard_layout.h:52`
- **Function**: `std::string DOS_GetLoadedLayout()`
- **Returns**: Layout name as std::string, empty if none

**Compatibility**: REFACTORED

**Migration Strategy**:
1. **Direct replacement available**: `DOS_GetLoadedLayout()`
2. **Signature change**: Returns `std::string` instead of `const char*`
3. **Semantics identical**: Returns layout name or empty/NULL
4. **Effort**: LOW (1-2 hours) - Simple wrapper function

---

### INT-065: boxer_keyboardLayoutSupported

**Legacy Implementation**:
- **Location**: `dos_keyboard_layout.cpp:1157-1169`
- **Purpose**: Check if layout can be loaded without code page change
- **Signature**: `bool boxer_keyboardLayoutSupported(const char *code)`
- **Behavior**:
  - Returns `true` if current layout supports language code
  - Returns `true` if layout can be loaded with current code page
  - Returns `false` otherwise

**Target Equivalent**:
- **Status**: MISSING
- **Location**: NOT FOUND
- **Changes**: No pre-check API; must attempt load to determine compatibility

**Compatibility**: MISSING

**Migration Strategy**:
1. **No direct replacement available**
2. **Can be emulated** via `ExtractCodePage()` (private method)
3. **Option A - Expose ExtractCodePage**: Make method public
4. **Option B - Trial load**: Attempt load, check result code
5. **Option C - Remove check**: Always attempt load
6. **Recommendation**: Option B (use `KeyboardLayoutResult` enum)
7. **Effort**: MEDIUM (4-6 hours) - Need to refactor private methods or implement trial-load logic

---

### INT-066: boxer_keyboardLayoutActive

**Legacy Implementation**:
- **Location**: `dos_keyboard_layout.cpp:1171-1176`
- **Purpose**: Check if foreign (non-US) layout is currently active
- **Signature**: `bool boxer_keyboardLayoutActive()`
- **Returns**: `true` if foreign layout is active, `false` if US or none
- **Behavior**: Returns `loaded_layout->foreign_layout_active()` or `false`

**Target Equivalent**:
- **Status**: MISSING
- **Location**: NOT FOUND
- **Changes**: No US/foreign toggle concept in target; layouts are always active when loaded

**Compatibility**: MISSING

**Migration Strategy**:
1. **Concept removed in target**: No US/foreign layout toggle
2. **Can be emulated**: Check if `DOS_GetLoadedLayout() != "US"` and not empty
3. **Semantic difference**: Target doesn't support runtime toggle without reload
4. **Option A - Remove feature**: Boxer adapts to simpler model
5. **Option B - Track state in Boxer**: Maintain toggle state in Boxer layer
6. **Recommendation**: Option A (simpler); Option B if toggle is critical
7. **Effort**: MEDIUM (4-8 hours) - Requires Boxer UI/logic changes

---

### INT-067: boxer_setKeyboardLayoutActive

**Legacy Implementation**:
- **Location**: `dos_keyboard_layout.cpp:1178-1189`
- **Purpose**: Activate or deactivate foreign keyboard layout
- **Signature**: `void boxer_setKeyboardLayoutActive(bool active)`
- **Behavior**:
  - Force-disables US layouts
  - Toggles `use_foreign_layout` flag without reloading
  - Calls `switch_foreign_layout()` if state changes

**Target Equivalent**:
- **Status**: MISSING
- **Location**: NOT FOUND
- **Changes**: No runtime toggle; layouts must be loaded/unloaded

**Compatibility**: MISSING

**Migration Strategy**:
1. **Feature removed in target**: No hot-swappable US/foreign toggle
2. **Alternative**: Reload layout or restore US layout via `DOS_LoadKeyboardLayout("US", ...)`
3. **Performance impact**: Full layout reload vs. toggle flag
4. **Option A - Accept reload cost**: Call `DOS_LoadKeyboardLayout()` to switch
5. **Option B - Reintroduce toggle**: Patch target to restore toggle functionality
6. **Recommendation**: Option A (simpler); Option B if performance-critical
7. **Effort**: MEDIUM (6-10 hours) for Option A; HIGH (15-20 hours) for Option B

---

### INT-068: boxer_setNumLockActive

**Legacy Implementation**:
- **Location**: `bios_keyboard.cpp:345`
- **Purpose**: Notify Boxer when Num Lock state changes in DOS
- **Signature**: `void boxer_setNumLockActive(bool active)`
- **Called from**: IRQ1 handler when scancode 0xc5 (Num Lock release) processed
- **Behavior**: Passes DOS Num Lock state to Boxer for macOS synchronization

**Target Equivalent**:
- **Status**: MISSING
- **Location**: NOT FOUND
- **Changes**: Lock key state tracked but not exposed via callbacks

**Compatibility**: MISSING

**Migration Strategy**:
1. **No callback mechanism in target**
2. **Can query state**: `KEYBOARD_GetLedState()` returns bit 1 for Num Lock
3. **Option A - Polling**: Periodically check `KEYBOARD_GetLedState()`
4. **Option B - Event hooks**: Add callback support to target BIOS keyboard handler
5. **Option C - Remove sync**: Boxer accepts desynchronized lock keys
6. **Recommendation**: Option A for quick solution; Option B for proper integration
7. **Effort**: LOW (2-3 hours) for Option A; MEDIUM (8-12 hours) for Option B

---

### INT-069: boxer_setCapsLockActive

**Legacy Implementation**:
- **Location**: `bios_keyboard.cpp:311`
- **Purpose**: Notify Boxer when Caps Lock state changes in DOS
- **Signature**: `void boxer_setCapsLockActive(bool active)`
- **Called from**: IRQ1 handler when scancode 0xba (Caps Lock release) processed
- **Behavior**: Passes DOS Caps Lock state to Boxer for macOS synchronization

**Target Equivalent**:
- **Status**: MISSING
- **Location**: NOT FOUND
- **Changes**: Lock key state tracked but not exposed via callbacks

**Compatibility**: MISSING

**Migration Strategy**:
1. **Same as INT-068** (Num Lock synchronization)
2. **Can query state**: `KEYBOARD_GetLedState()` returns bit 2 for Caps Lock
3. **Option A - Polling**: Periodically check `KEYBOARD_GetLedState()`
4. **Option B - Event hooks**: Add callback support to target BIOS keyboard handler
5. **Option C - Remove sync**: Boxer accepts desynchronized lock keys
6. **Recommendation**: Option A for quick solution; Option B for proper integration
7. **Effort**: LOW (2-3 hours) for Option A; MEDIUM (8-12 hours) for Option B

---

### INT-070: boxer_setScrollLockActive

**Legacy Implementation**:
- **Location**: `bios_keyboard.cpp:350`
- **Purpose**: Notify Boxer when Scroll Lock state changes in DOS
- **Signature**: `void boxer_setScrollLockActive(bool active)`
- **Called from**: IRQ1 handler when scancode 0xc6 (Scroll Lock release) processed
- **Behavior**: Passes DOS Scroll Lock state to Boxer for macOS synchronization

**Target Equivalent**:
- **Status**: MISSING
- **Location**: NOT FOUND
- **Changes**: Lock key state tracked but not exposed via callbacks

**Compatibility**: MISSING

**Migration Strategy**:
1. **Same as INT-068, INT-069** (lock key synchronization)
2. **Can query state**: `KEYBOARD_GetLedState()` returns bit 0 for Scroll Lock
3. **Option A - Polling**: Periodically check `KEYBOARD_GetLedState()`
4. **Option B - Event hooks**: Add callback support to target BIOS keyboard handler
5. **Option C - Remove sync**: Boxer accepts desynchronized lock keys
6. **Recommendation**: Option A for quick solution; Option B for proper integration
7. **Effort**: LOW (2-3 hours) for Option A; MEDIUM (8-12 hours) for Option B

**Note**: All three lock keys (INT-068, INT-069, INT-070) should use the same integration strategy for consistency.

---

### INT-071: boxer_preferredKeyboardLayout

**Legacy Implementation**:
- **Location**: `dos_keyboard_layout.cpp:1350`
- **Purpose**: Let Boxer provide preferred keyboard layout during auto-detection
- **Signature**: `const char* boxer_preferredKeyboardLayout()`
- **Called from**: DOS_KeyboardLayout constructor when layout="auto"
- **Behavior**: Boxer returns layout string (e.g., "fr", "de") based on macOS settings

**Target Equivalent**:
- **Status**: MISSING
- **Location**: NOT FOUND
- **Changes**: Auto-detection removed; target requires explicit layout specification

**Compatibility**: MISSING

**Migration Strategy**:
1. **Feature removed in target**: No auto-detection mechanism
2. **Option A - Boxer pre-configuration**: Boxer sets layout in config before DOSBox init
3. **Option B - Manual specification**: User must specify layout in DOSBox config
4. **Option C - Restore hook**: Add callback support to target layout initialization
5. **Recommendation**: Option A (Boxer configures DOSBox programmatically)
6. **Effort**: MEDIUM (4-6 hours) - Requires Boxer to generate/modify DOSBox config

---

### INT-072: boxer_continueListeningForKeyEvents

**Legacy Implementation**:
- **Location**: `bios_keyboard.cpp:488, dev_con.h:78, shell_misc.cpp:76`
- **Purpose**: Allow Boxer to interrupt DOS keyboard polling loops
- **Signature**: `bool boxer_continueListeningForKeyEvents()`
- **Called from**: INT 16h handler (GET KEYSTROKE, CHECK FOR KEYSTROKE)
- **Behavior**: Returns `false` to interrupt polling, `true` to continue
- **Result**: Returns CBRET_STOP to exit INT 16h handler early

**Target Equivalent**:
- **Status**: MISSING
- **Location**: NOT FOUND
- **Changes**: No interruption mechanism in INT 16h handler

**Compatibility**: MISSING

**Migration Strategy**:
1. **No callback mechanism in target**
2. **Use case**: Boxer needs to interrupt DOS when user clicks away from window
3. **Option A - Remove feature**: DOS programs may hang in keyboard loops
4. **Option B - Inject key**: Force DOS to see a key press (e.g., ESC)
5. **Option C - Add hook**: Patch target INT 16h handler to support interruption
6. **Option D - Shutdown detection**: Use existing `DOSBOX_IsShutdownRequested()` pattern
7. **Recommendation**: Option B for quick solution; Option C for proper integration
8. **Effort**: LOW (2-4 hours) for Option B; MEDIUM (8-12 hours) for Option C

---

### INT-073: boxer_numKeyCodesInPasteBuffer

**Legacy Implementation**:
- **Location**: `dev_con.h:414` (NOT FOUND IN TARGET)
- **Purpose**: Query number of keycodes remaining in paste buffer
- **Signature**: `int boxer_numKeyCodesInPasteBuffer()`
- **Returns**: Number of paste keycodes not yet consumed by DOS
- **Usage**: Allows Boxer to track paste progress

**Target Equivalent**:
- **Status**: MISSING
- **Location**: NOT FOUND
- **Changes**: No paste buffer system in target

**Compatibility**: MISSING

**Migration Strategy**:
1. **Entire paste buffer system missing**
2. **Option A - Implement in Boxer wrapper**: Inject keys via `KEYBOARD_AddKey()` sequence
3. **Option B - Add paste buffer to target**: Recreate paste buffer in BIOS keyboard
4. **Option C - Clipboard integration**: Add native clipboard paste support to target
5. **Recommendation**: Option A (Boxer-side implementation) for independence
6. **Effort**: MEDIUM (8-12 hours) for Option A; HIGH (20-30 hours) for Option B/C

---

### INT-074: boxer_getNextKeyCodeInPasteBuffer

**Legacy Implementation**:
- **Location**: `bios_keyboard.cpp:162,188`
- **Purpose**: Retrieve next keycode from paste buffer
- **Signature**: `bool boxer_getNextKeyCodeInPasteBuffer(uint16_t* code, bool consume)`
- **Parameters**:
  - `code`: Output parameter for keycode
  - `consume`: If true, remove keycode from buffer; if false, peek only
- **Returns**: `true` if keycode available, `false` if buffer empty
- **Called from**: `get_key()` and `check_key()` in BIOS keyboard handler
- **Priority**: Paste buffer checked BEFORE DOS keyboard buffer

**Target Equivalent**:
- **Status**: MISSING
- **Location**: NOT FOUND
- **Changes**: No paste buffer system in target

**Compatibility**: MISSING

**Migration Strategy**:
1. **Same as INT-073** (paste buffer system missing)
2. **Integrated approach required**: Both query and retrieve functions needed
3. **Option A - Boxer-side queue**: Maintain paste queue in Boxer, inject via `KEYBOARD_AddKey()`
4. **Option B - Target-side integration**: Add paste buffer to target's BIOS keyboard
5. **Option C - SDL clipboard events**: Use SDL clipboard integration (if target supports)
6. **Recommendation**: Option A for Boxer control; Option B for target-native solution
7. **Effort**: MEDIUM (8-12 hours) for Option A; HIGH (20-30 hours) for Option B
8. **Note**: Should be implemented together with INT-073

---

## Summary Table

| ID | Name | Category | Target Status | Complexity | Effort (hours) |
|----|------|----------|---------------|------------|----------------|
| INT-060 | setJoystickActive | Joystick | MISSING | MEDIUM | 4-6 |
| INT-062 | keyboardBufferRemaining | Keyboard | MISSING | LOW | 2-4 |
| INT-063 | keyboardLayoutLoaded | Layout | REFACTORED | LOW | 1-2 |
| INT-064 | keyboardLayoutName | Layout | REFACTORED | LOW | 1-2 |
| INT-065 | keyboardLayoutSupported | Layout | MISSING | MEDIUM | 4-6 |
| INT-066 | keyboardLayoutActive | Layout | MISSING | MEDIUM | 4-8 |
| INT-067 | setKeyboardLayoutActive | Layout | MISSING | MEDIUM-HIGH | 6-10 |
| INT-068 | setNumLockActive | Lock Keys | MISSING | LOW-MEDIUM | 2-3 (poll) / 8-12 (hook) |
| INT-069 | setCapsLockActive | Lock Keys | MISSING | LOW-MEDIUM | 2-3 (poll) / 8-12 (hook) |
| INT-070 | setScrollLockActive | Lock Keys | MISSING | LOW-MEDIUM | 2-3 (poll) / 8-12 (hook) |
| INT-071 | preferredKeyboardLayout | Layout | MISSING | MEDIUM | 4-6 |
| INT-072 | continueListeningForKeyEvents | Input Control | MISSING | LOW-MEDIUM | 2-4 (inject) / 8-12 (hook) |
| INT-073 | numKeyCodesInPasteBuffer | Paste | MISSING | MEDIUM-HIGH | 8-12 (Boxer) / 20-30 (target) |
| INT-074 | getNextKeyCodeInPasteBuffer | Paste | MISSING | MEDIUM-HIGH | 8-12 (Boxer) / 20-30 (target) |

**Total Points**: 15
**Refactored (easy)**: 2 (13%)
**Missing (moderate-hard)**: 13 (87%)

## Feature Compatibility

### Keyboard Layouts

**Status**: Partially Supported

**Target Capabilities**:
- ✅ Load keyboard layouts from .kl or .sys files
- ✅ Bundled FreeDOS keyboard layouts (KEYBOARD.SYS, KEYBRD2-4.SYS)
- ✅ Code page integration with screen fonts
- ✅ Diacritics and submapping support
- ✅ Query loaded layout name
- ❌ Runtime US/foreign toggle
- ❌ Layout compatibility pre-check
- ❌ Auto-detection from host system
- ❌ External configuration hooks

**Migration**:
- **Direct API replacement**: `DOS_GetLoadedLayout()` replaces name queries
- **Removed features**: US/foreign toggle, auto-detection, pre-load validation
- **Boxer adaptation required**: Must reload layouts instead of toggling
- **Configuration**: Boxer must pre-configure layout via DOSBox config file

### Lock Key Sync

**Status**: Not Supported

**Target Capabilities**:
- ✅ Track lock key states internally (`KEYBOARD_GetLedState()`)
- ✅ Proper BIOS flag management
- ✅ DOS programs can read lock key states
- ❌ No event callbacks when lock keys change
- ❌ No push notifications to external clients

**Migration**:
- **Polling approach**: Boxer can poll `KEYBOARD_GetLedState()` periodically
- **Event-driven approach**: Requires patching target to add callbacks
- **Trade-offs**: Polling adds CPU overhead but avoids target modifications
- **Recommendation**: Start with polling, add events if performance issues arise

### Paste Buffer

**Status**: Not Supported

**Target Capabilities**:
- ✅ Keyboard input via `KEYBOARD_AddKey()`
- ✅ Scancode generation for all keys
- ✅ Typematic (key repeat) support
- ❌ No clipboard integration
- ❌ No paste buffer system
- ❌ No bulk key injection API

**Migration**:
- **Boxer-side implementation**: Convert clipboard text to key sequence, inject via `KEYBOARD_AddKey()`
- **Challenges**:
  - Timing: Must space keys realistically
  - Buffer overflow: Keyboard buffer is only 8 scancodes deep
  - Text conversion: Must convert Unicode to DOS code page
  - Special keys: Shift state management for uppercase/symbols
- **Alternative**: Patch target to add paste buffer to BIOS keyboard
- **Recommendation**: Implement in Boxer for independence from target changes

### Joystick

**Status**: Partially Supported

**Target Capabilities**:
- ✅ Full gameport emulation (timed and count-based modes)
- ✅ Joystick enable/disable (`JOYSTICK_Enable()`)
- ✅ Visibility control (`JOY_ONLY_FOR_MAPPING` type)
- ✅ Calibration support (X/Y axis tuning)
- ✅ Query joystick accessibility (`JOYSTICK_IsAccessible()`)
- ❌ No activation notification callbacks

**Migration**:
- **Polling approach**: Periodically check `JOYSTICK_IsAccessible()`
- **I/O hook approach**: Wrap I/O port handlers to detect gameport access
- **Recommendation**: I/O hook approach for real-time detection

## Migration Complexity

### Total Effort: 40-60 hours

**Breakdown by Feature**:

1. **Keyboard layout system** (10-15 hours):
   - API wrapper for layout queries: 2 hours
   - Remove US/foreign toggle logic in Boxer: 4-6 hours
   - Implement layout reload instead of toggle: 4-6 hours
   - Config file generation for auto-detection: 2-3 hours

2. **Lock keys** (6-24 hours depending on approach):
   - **Polling approach** (LOW complexity): 6-9 hours
     - Implement polling loop: 2-3 hours
     - State change detection: 2-3 hours
     - macOS LED synchronization: 2-3 hours
   - **Event hook approach** (MEDIUM complexity): 16-24 hours
     - Patch target BIOS keyboard handler: 8-12 hours
     - Callback infrastructure: 4-6 hours
     - macOS LED synchronization: 4-6 hours

3. **Paste buffer** (12-30 hours depending on approach):
   - **Boxer-side approach** (MEDIUM complexity): 12-18 hours
     - Clipboard text conversion: 4-6 hours
     - Key sequence injection: 4-6 hours
     - Buffer management and timing: 4-6 hours
   - **Target-side approach** (HIGH complexity): 20-30 hours
     - Implement paste buffer in target BIOS: 10-15 hours
     - Clipboard integration: 6-10 hours
     - Testing and edge cases: 4-5 hours

4. **Joystick activation** (4-6 hours):
   - I/O port hook implementation: 2-3 hours
   - Activation state tracking: 1-2 hours
   - UI integration: 1-1 hour

5. **Input continuation** (4-8 hours):
   - Key injection approach: 4 hours
   - Hook-based approach: 8 hours

6. **Keyboard buffer query** (2-4 hours):
   - Remove Boxer dependency: 1-2 hours
   - Add query API to target (if needed): 1-2 hours

### Risk Factors

**Technical complexity**:
- Paste buffer requires careful timing and code page conversion
- Lock key polling adds CPU overhead
- Keyboard layout toggle removal may affect Boxer UX

**Testing requirements**:
- International keyboards (French, German, Spanish, etc.)
- Clipboard paste with special characters and accents
- Lock key synchronization across window focus changes
- Joystick hot-plug and calibration

## Risks

### HIGH Risks

1. **Paste functionality may break or behave poorly**
   - **Issue**: Target has no paste buffer system
   - **Impact**: Users cannot paste text into DOS programs
   - **Mitigation**: Implement robust Boxer-side paste with throttling

2. **Keyboard layout switching requires full reload**
   - **Issue**: No US/foreign toggle, must reload entire layout
   - **Impact**: Slower layout switching, potential flicker
   - **Mitigation**: Cache layouts in Boxer, optimize reload path

3. **Lock key desynchronization**
   - **Issue**: Polling introduces lag between DOS and macOS state
   - **Impact**: LEDs may be out of sync for 100-500ms
   - **Mitigation**: High-frequency polling (10-20 Hz) or event hooks

### MEDIUM Risks

4. **Joystick activation detection may be delayed**
   - **Issue**: Polling or I/O hooks add overhead
   - **Impact**: Slight delay in showing joystick UI
   - **Mitigation**: Optimize polling frequency or hook implementation

5. **Keyboard buffer overflow with paste**
   - **Issue**: Target keyboard buffer is only 8 scancodes
   - **Impact**: Fast paste may lose characters
   - **Mitigation**: Throttle paste rate, monitor buffer state (if API added)

6. **Code page conversion errors in paste**
   - **Issue**: Unicode to DOS code page conversion may fail
   - **Impact**: Some characters won't paste correctly
   - **Mitigation**: Robust fallback (e.g., replace unmappable chars with '?')

## Recommendations

### Priority 1 (Critical - Must Implement)

1. **Implement Boxer-side paste buffer** (12-18 hours)
   - Convert clipboard text to DOS keycode sequence
   - Inject keys via `KEYBOARD_AddKey()` with throttling
   - Handle code page conversion and special characters
   - **Rationale**: Paste is user-facing feature, critical for productivity

2. **Replace keyboard layout toggle with reload** (6-8 hours)
   - Remove US/foreign toggle UI/logic from Boxer
   - Implement layout reload via `DOS_LoadKeyboardLayout()`
   - Update Boxer UI to reflect new model
   - **Rationale**: Layout system fundamentally changed; toggle unsupported

3. **Wrap keyboard layout query APIs** (2 hours)
   - Create Boxer wrapper for `DOS_GetLoadedLayout()`
   - Replace all `boxer_keyboardLayout*()` calls with wrappers
   - **Rationale**: Direct API replacement available, minimal effort

### Priority 2 (Important - Should Implement)

4. **Implement lock key synchronization** (6-9 hours, polling approach)
   - Poll `KEYBOARD_GetLedState()` at 10-20 Hz
   - Detect state changes and update macOS LEDs
   - **Rationale**: Lock key sync is expected feature for integration

5. **Implement joystick activation detection** (4-6 hours)
   - Wrap I/O port handlers or poll `JOYSTICK_IsAccessible()`
   - Update Boxer joystick UI visibility
   - **Rationale**: Improves user experience, manageable complexity

6. **Implement auto-layout configuration** (4-6 hours)
   - Boxer generates DOSBox config with correct keyboard layout
   - Detect macOS keyboard layout on startup
   - **Rationale**: Preserves auto-detection functionality

### Priority 3 (Optional - Consider If Time Permits)

7. **Add event hooks for lock keys** (16-24 hours)
   - Patch target BIOS keyboard to call callbacks
   - Eliminates polling overhead
   - **Rationale**: Better performance, but requires target modifications

8. **Add paste buffer to target DOSBox** (20-30 hours)
   - Implement native paste buffer in BIOS keyboard
   - Proper clipboard integration
   - **Rationale**: Better solution long-term, but high complexity

9. **Add input continuation hooks** (8-12 hours)
   - Patch target INT 16h handler to support interruption
   - **Rationale**: Prevents DOS hang, but may not be essential

### Alternative Approaches

**For lock key sync**:
- If polling overhead is problematic, implement event hooks in target
- If synchronization is non-critical, remove feature entirely

**For paste**:
- If Boxer-side implementation is insufficient, contribute paste buffer to DOSBox Staging upstream
- Consider SDL2 clipboard event integration for better platform support

**For keyboard layouts**:
- If toggle is critical, maintain fork with toggle patch
- Otherwise, accept reload model and optimize reload performance

## Blockers/Open Questions

### Blockers

1. **dev_con.h missing from target**
   - **Question**: Was paste buffer intentionally removed or refactored?
   - **Action**: Search target codebase for clipboard/paste functionality
   - **Impact**: May indicate paste is implemented differently

2. **No documentation for input continuation use cases**
   - **Question**: Why does Boxer need to interrupt INT 16h polling?
   - **Action**: Review Boxer code to understand interruption scenarios
   - **Impact**: May determine if feature is necessary

### Open Questions

1. **Keyboard buffer size in target**: Observed 8 scancodes, but how much space for multi-byte scancodes?
   - **Action**: Test buffer limits with extended keys (e.g., arrow keys = 2 bytes)

2. **Lock key polling frequency**: What polling rate provides good responsiveness without CPU overhead?
   - **Action**: Benchmark various frequencies (5 Hz, 10 Hz, 20 Hz, 50 Hz)

3. **Paste throttle rate**: What key injection rate avoids buffer overflow?
   - **Action**: Experiment with delays (e.g., 10ms, 20ms, 50ms per key)

4. **Code page conversion in target**: Does target provide Unicode to DOS code page conversion API?
   - **Action**: Search target for code page conversion utilities

5. **Joystick detection timing**: How quickly does Boxer need to detect joystick usage?
   - **Action**: Review Boxer UI update requirements

### Investigation Needed

1. **Search target for clipboard functionality**:
   ```bash
   grep -r "clipboard\|paste\|SDL_GetClipboard" /home/user/dosbox-staging/src/
   ```

2. **Check target code page conversion APIs**:
   ```bash
   grep -r "code.*page.*convert\|cp437\|unicode" /home/user/dosbox-staging/src/dos/
   ```

3. **Review target keyboard buffer implementation**:
   - Read `/home/user/dosbox-staging/src/hardware/input/keyboard.cpp` buffer[] array
   - Determine actual capacity with multi-byte scancodes

4. **Analyze target build system**:
   - Check if BXCoalface.h or Boxer hooks exist in different branch
   - Verify target branch is correct (`dosbox-boxer-upgrade-dosboxside`)

5. **Test target keyboard layout loading**:
   - Verify `DOS_LoadKeyboardLayout()` works correctly
   - Test reload performance (important for toggle replacement)
