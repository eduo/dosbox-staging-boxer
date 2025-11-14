# Boxer-DOSBox Integration Mapping

**Version:** 1.0
**Date:** 2025-11-14
**Purpose:** Detailed technical mapping of all Boxer integration points

## Overview

This document provides a complete technical reference for all modifications made to DOSBox Staging to support Boxer integration. It serves as a guide for porting the integration from version 0.78.0 to 0.83.0-alpha.

## Integration Architecture

```
┌─────────────────────────────────────────────┐
│           Boxer Application                 │
│         (Objective-C/Swift)                 │
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│         BXEmulator (Objective-C++)          │
│    Main emulator controller class           │
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│      BXCoalface.mm (Objective-C++)          │
│   C function wrappers → Obj-C methods       │
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│        BXCoalface.h (C/C++)                 │
│  - Callback function declarations           │
│  - Macro-based function replacements        │
│  - Extern "C" linkage                       │
└───────────────┬─────────────────────────────┘
                │
                ▼
┌─────────────────────────────────────────────┐
│       DOSBox Staging (C++20)                │
│   Modified source files with hooks          │
└─────────────────────────────────────────────┘
```

## File Modifications by Category

### 1. Core Engine

#### src/dosbox.cpp - Main Emulation Loop

**Purpose:** Control emulation execution from Boxer

**Location (Old):** `src/dosbox.cpp`
**Location (New):** `src/dosbox.cpp` (unchanged)

**Modifications:**

```cpp
// Line ~160, 179: Main loop control
void DOSBOX_RunMachine() {
    // ... setup code ...

    // BOXER MODIFICATION: Allow Boxer to terminate loop
    while (boxer_runLoopShouldContinue()) {
        // ... emulation cycle ...
    }
}

// Line ~353, 355: Runloop lifecycle tracking
void RunDOSBox() {
    // ... initialization ...

    // BOXER MODIFICATION: Notify start
    boxer_runLoopWillStartWithContextInfo(context_info);

    DOSBOX_RunMachine();

    // BOXER MODIFICATION: Notify finish
    boxer_runLoopDidFinishWithContextInfo(context_info);

    // ... cleanup ...
}
```

**Callbacks:**
```cpp
bool boxer_runLoopShouldContinue();
void boxer_runLoopWillStartWithContextInfo(void* context);
void boxer_runLoopDidFinishWithContextInfo(void* context);
```

**Migration Notes:**
- API stable, no changes expected
- Context info allows nested runloop tracking
- Simple boolean check, minimal performance impact

---

### 2. Rendering System

#### src/gui/render/render.cpp - Frame Rendering

**Purpose:** Capture frame data for Boxer's custom rendering

**Location (Old):** `src/gui/render.cpp`
**Location (New):** `src/gui/render/render.cpp` ⚠️ **PATH CHANGED**

**Modifications:**

```cpp
// In rendering setup
uint8_t GFX_SetSize(int width, int height, Fraction aspect_ratio,
                    uint8_t flags, VideoMode& mode, GFX_Callback_t callback) {
    // ... standard setup ...

    // BOXER MODIFICATION: Let Boxer prepare for new frame size
    return boxer_prepareForFrameSize(width, height, aspect_ratio,
                                      flags, mode, callback);
}

// In frame rendering
bool GFX_StartUpdate(uint8_t*& pixels, int& pitch) {
    // BOXER MODIFICATION: Get frame buffer from Boxer
    return boxer_startFrame(&pixels, &pitch);
}

void GFX_EndUpdate() {
    // BOXER MODIFICATION: Notify Boxer frame is complete
    boxer_finishFrame();
}
```

**Callbacks (Old 0.78.0):**
```cpp
void boxer_prepareForFrameSize(int width, int height, float aspect,
                                uint8_t flags, void* mode, void* callback);
void boxer_startFrame();
void boxer_finishFrame();
```

**Callbacks (New 0.83.0):** ⚠️ **SIGNATURES CHANGED**
```cpp
uint8_t boxer_prepareForFrameSize(int width, int height,
                                    Fraction aspect_ratio,  // Changed type
                                    uint8_t flags,
                                    VideoMode& mode,  // Now a reference
                                    GFX_Callback_t callback);

bool boxer_startFrame(uint8_t** pixels_out, int* pitch_out);  // Added params
void boxer_finishFrame();
```

**Migration Notes:**
- ⚠️ **BREAKING:** Aspect ratio now `Fraction` type instead of `float`
- ⚠️ **BREAKING:** `startFrame` now returns buffer pointers
- Need to convert: `float aspect = (float)aspect_ratio.numerator / aspect_ratio.denominator`
- Consider implementing `BoxerRenderBackend` class for cleaner integration

**Recommended Approach:**
```cpp
// In BoxerRenderBackend::StartFrame
void BoxerRenderBackend::StartFrame(uint8_t*& pixels_out, int& pitch_out) {
    bool success = boxer_startFrame(&framebuffer_, &pitch_);
    if (success) {
        pixels_out = framebuffer_;
        pitch_out = pitch_;
    }
}
```

#### src/hardware/video/vga_other.cpp - Display Modes

**Purpose:** Expose CGA/Hercules mode controls to Boxer UI

**Location (Old):** `src/hardware/vga_other.cpp`
**Location (New):** `src/hardware/video/vga_other.cpp` ⚠️ **PATH CHANGED**

**Modifications:**

```cpp
// Lines 1454-1470: Hercules tint mode
uint8_t VGA_GetHerculesTintMode() {
    return hercules_tint_mode;
}

void VGA_SetHerculesTintMode(uint8_t mode) {
    hercules_tint_mode = mode;
    VGA_SetupHandlers();
}

// Lines 1472-1487: CGA composite hue offset
float VGA_GetCGACompositeHueOffset() {
    return cga_hue_offset;
}

void VGA_SetCGACompositeHueOffset(float offset) {
    cga_hue_offset = offset;
    VGA_RecalculateCGAPalette();
}

// Lines 1490-1498: CGA component mode
bool VGA_GetCGAComponentMode() {
    return cga_component_mode;
}

void VGA_SetCGAComponentMode(bool enabled) {
    cga_component_mode = enabled;
    VGA_RecalculateCGAPalette();
}
```

**Callbacks:**
```cpp
uint8_t boxer_herculesTintMode();
void boxer_setHerculesTintMode(uint8_t mode);

float boxer_CGACompositeHueOffset();
void boxer_setCGACompositeHueOffset(float offset);

bool boxer_CGAComponentMode();
void boxer_setCGAComponentMode(bool enabled);
```

**Migration Notes:**
- Simple getter/setter functions
- Path changed: `hardware/` → `hardware/video/`
- API stable, no signature changes

---

### 3. Shell Integration

#### src/shell/shell.cpp - Shell Lifecycle

**Purpose:** Track shell startup/shutdown and autoexec

**Location (Old):** `src/shell/shell.cpp`
**Location (New):** `src/shell/shell.cpp` (unchanged)

**Modifications:**

```cpp
void DOS_Shell::Run() {
    // BOXER MODIFICATION: Notify shell started
    boxer_shellWillStart();

    // ... shell main loop ...

    // BOXER MODIFICATION: Notify shell finished
    boxer_shellDidFinish();
}

void DOS_Shell::RunAutoexec() {
    // BOXER MODIFICATION: Notify autoexec starting
    boxer_shellWillStartAutoexec();

    // ... execute autoexec.bat ...

    // BOXER MODIFICATION: Notify returned to shell
    boxer_didReturnToShell();
}
```

**Callbacks:**
```cpp
void boxer_shellWillStart();
void boxer_shellDidFinish();
void boxer_shellWillStartAutoexec();
void boxer_didReturnToShell();
```

**Migration Notes:**
- Straightforward lifecycle hooks
- No changes expected

#### src/shell/shell_cmds.cpp - Command Interception

**Purpose:** Allow Boxer to handle commands before DOSBox

**Location (Old):** `src/shell/shell_cmds.cpp`
**Location (New):** `src/shell/shell_cmds.cpp` (unchanged)

**Modifications:**

```cpp
// Line ~182-183: Command execution hook
void DOS_Shell::ExecuteCommand(const char* command) {
    // BOXER MODIFICATION: Let Boxer intercept command
    if (!boxer_shellShouldRunCommand(command)) {
        return;  // Boxer handled it
    }

    // ... standard command processing ...
}
```

**Callbacks:**
```cpp
bool boxer_shellShouldRunCommand(const char* command);
```

**Returns:** `false` if Boxer handled the command, `true` to continue

**Migration Notes:**
- Critical for Boxer's command injection
- No changes expected

#### src/shell/shell_misc.cpp - Input and Execution Tracking

**Purpose:** Monitor user input and program execution

**Location (Old):** `src/shell/shell_misc.cpp`
**Location (New):** `src/shell/shell_misc.cpp` (unchanged)

**Modifications:**

```cpp
// Lines 75-82: Command input wrapper
void DOS_Shell::GetCommand(char* buffer, size_t max_len) {
    // BOXER MODIFICATION: Notify will read input
    boxer_shellWillReadCommandInputFromHandle(stdin_handle);

    // Read input
    DOS_ReadFile(stdin_handle, buffer, &bytes_read);

    // BOXER MODIFICATION: Notify did read input
    boxer_shellDidReadCommandInputFromHandle(stdin_handle);
}

// Lines 84-86: Shell continuation check
bool DOS_Shell::ShouldContinue() {
    // BOXER MODIFICATION: Let Boxer terminate shell
    return boxer_shellShouldContinue();
}

// Lines 90-96: Command input modification
void DOS_Shell::ProcessInput(char* buffer) {
    // BOXER MODIFICATION: Let Boxer modify input
    boxer_handleShellCommandInput(buffer, max_length);

    // ... process modified input ...
}

// Line 557: Program execution tracking
void DOS_Shell::ExecuteProgram(const char* name) {
    // BOXER MODIFICATION: Notify program execution
    boxer_shellWillExecuteFileAtDOSPath(name);

    // ... execute program ...
}
```

**Callbacks:**
```cpp
void boxer_shellWillReadCommandInputFromHandle(uint16_t handle);
void boxer_shellDidReadCommandInputFromHandle(uint16_t handle);
bool boxer_shellShouldContinue();
void boxer_handleShellCommandInput(char* buffer, size_t max_len);
void boxer_shellWillExecuteFileAtDOSPath(const char* path);
```

**Migration Notes:**
- Provides complete visibility into shell operations
- No changes expected

#### src/shell/shell_batch.cpp - Batch File Tracking

**Purpose:** Monitor batch file execution

**Location (Old):** `src/shell/shell_batch.cpp`
**Location (New):** `src/shell/shell_batch.cpp` (unchanged)

**Modifications:**

```cpp
// Line ~542: Batch start
void BatchFile::Start(const char* filename) {
    // BOXER MODIFICATION: Notify batch started
    boxer_shellWillBeginBatchFile(filename);

    // ... start batch ...
}

// Lines 547-548, 73: Batch end
void BatchFile::Close() {
    // BOXER MODIFICATION: Notify batch ended
    boxer_shellDidEndBatchFile(filename_);

    // ... cleanup ...
}
```

**Callbacks:**
```cpp
void boxer_shellWillBeginBatchFile(const char* filename);
void boxer_shellDidEndBatchFile(const char* filename);
```

**Migration Notes:**
- Simple lifecycle tracking
- No changes expected

---

### 4. File System

#### src/dos/drive_local.cpp - Local Drive Operations

**Purpose:** Control file access and track changes

**Location (Old):** `src/dos/drive_local.cpp`
**Location (New):** `src/dos/drive_local.cpp` (unchanged)

**Modifications:**

```cpp
// Lines 60-62: Write protection
bool localDrive::FileOpen(DOS_File** file, const char* name, uint32_t flags) {
    if (flags & OPEN_WRITE) {
        // BOXER MODIFICATION: Check write permission
        if (!boxer_shouldAllowWriteAccessToPath(GetFullPath(name))) {
            DOS_SetError(DOSERR_ACCESS_DENIED);
            return false;
        }
    }

    // ... open file ...
}

// Line 86: File creation notification
bool localDrive::FileCreate(DOS_File** file, const char* name, uint16_t attributes) {
    bool success = CreateFileInHost(name, attributes);

    if (success) {
        // BOXER MODIFICATION: Notify file created
        boxer_didCreateLocalFile(GetFullPath(name));
    }

    return success;
}

// Lines 265-267, 276, 297: File deletion
bool localDrive::FileUnlink(const char* name) {
    // BOXER MODIFICATION: Check write permission
    if (!boxer_shouldAllowWriteAccessToPath(GetFullPath(name))) {
        DOS_SetError(DOSERR_ACCESS_DENIED);
        return false;
    }

    bool success = DeleteFileInHost(name);

    if (success) {
        // BOXER MODIFICATION: Notify file removed
        boxer_didRemoveLocalFile(GetFullPath(name));
    }

    return success;
}

// Line 450: Directory creation
bool localDrive::MakeDir(const char* name) {
    // BOXER MODIFICATION: Check write permission
    if (!boxer_shouldAllowWriteAccessToPath(GetFullPath(name))) {
        DOS_SetError(DOSERR_ACCESS_DENIED);
        return false;
    }

    return CreateDirectoryInHost(name);
}
```

**Callbacks:**
```cpp
bool boxer_shouldAllowWriteAccessToPath(const char* path);
void boxer_didCreateLocalFile(const char* path);
void boxer_didRemoveLocalFile(const char* path);
```

**Migration Notes:**
- Critical for Boxer's file protection
- No API changes expected
- Consider adding move/rename hooks if missing

#### src/dos/drive_cache.cpp - File Visibility

**Purpose:** Hide macOS system files from DOS

**Location (Old):** `src/dos/drive_cache.cpp`
**Location (New):** `src/dos/drive_cache.cpp` (unchanged)

**Modifications:**

```cpp
// In directory enumeration
void DriveCache::AddEntry(const char* filename) {
    // BOXER MODIFICATION: Filter system files
    if (!boxer_shouldShowFileWithName(filename)) {
        return;  // Hide this file
    }

    // ... add to cache ...
}
```

**Callbacks:**
```cpp
bool boxer_shouldShowFileWithName(const char* filename);
```

**Returns:** `false` to hide file (e.g., .DS_Store, .Spotlight-V100)

**Migration Notes:**
- Simple filtering
- No changes expected

#### src/dos/programs/mount.cpp - Mount Operations

**Purpose:** Track drive mounting and enforce restrictions

**Location (Old):** `src/dos/program_mount.cpp`
**Location (New):** `src/dos/programs/mount.cpp` ⚠️ **PATH CHANGED**

**Modifications:**

```cpp
// In MOUNT command handler
void MOUNT::Run() {
    // ... parse arguments ...

    // BOXER MODIFICATION: Check if mount allowed
    if (!boxer_shouldMountPath(path)) {
        WriteOut("Access denied");
        return;
    }

    // ... perform mount ...

    // BOXER MODIFICATION: Notify mount succeeded
    boxer_driveDidMount(drive_letter, path, type);
}

// In unmount
void MOUNT::Unmount(char drive) {
    // ... perform unmount ...

    // BOXER MODIFICATION: Notify unmount
    boxer_driveDidUnmount(drive);
}
```

**Callbacks:**
```cpp
bool boxer_shouldMountPath(const char* path);
void boxer_driveDidMount(char drive, const char* path, uint8_t type);
void boxer_driveDidUnmount(char drive);
```

**Migration Notes:**
- Path may have changed (verify in new version)
- Programs now in `src/dos/programs/` subdirectory
- Check if still `mount.cpp` or renamed

---

### 5. Input Handling

#### src/hardware/input/keyboard.cpp - Keyboard

**Purpose:** Provide access to keyboard buffer management

**Location (Old):** `src/hardware/keyboard.cpp`
**Location (New):** `src/hardware/input/keyboard.cpp` ⚠️ **PATH CHANGED**

**Modifications:**

```cpp
// Line 28: Include BXCoalface
#include "BXCoalface.h"

// Functions already exported, just need to ensure visibility
```

**Callbacks:** None directly in this file (see bios_keyboard.cpp)

**Migration Notes:**
- Simple path change
- Update include: `"hardware/input/keyboard.h"`

#### src/ints/bios_keyboard.cpp - Keyboard BIOS

**Purpose:** Keyboard paste, lock key sync, input interruption

**Location (Old):** `src/ints/bios_keyboard.cpp`
**Location (New):** `src/ints/bios_keyboard.cpp` (unchanged)

**Modifications:**

```cpp
// Lines 162-163, 188-189: Paste buffer integration
uint16_t BIOS_GetKeyCode() {
    // BOXER MODIFICATION: Check paste buffer first
    if (boxer_numKeyCodesInPasteBuffer() > 0) {
        return boxer_getNextKeyCodeInPasteBuffer();
    }

    // ... normal keyboard input ...
}

// Lines 311, 345, 350: Lock key state sync
void BIOS_SetCapsLock(bool state) {
    caps_lock_state = state;

    // BOXER MODIFICATION: Sync with macOS
    boxer_setCapsLockActive(state);
}

void BIOS_SetNumLock(bool state) {
    num_lock_state = state;

    // BOXER MODIFICATION: Sync with macOS
    boxer_setNumLockActive(state);
}

void BIOS_SetScrollLock(bool state) {
    scroll_lock_state = state;

    // BOXER MODIFICATION: Sync with macOS
    boxer_setScrollLockActive(state);
}

// Lines 488-490: Input loop interruption
void BIOS_WaitForKey() {
    while (true) {
        // BOXER MODIFICATION: Allow breaking out of wait loop
        if (!boxer_continueListeningForKeyEvents()) {
            return;
        }

        // ... check for key ...
    }
}
```

**Callbacks:**
```cpp
uint32_t boxer_numKeyCodesInPasteBuffer();
uint16_t boxer_getNextKeyCodeInPasteBuffer();
void boxer_setCapsLockActive(bool active);
void boxer_setNumLockActive(bool active);
void boxer_setScrollLockActive(bool active);
bool boxer_continueListeningForKeyEvents();
```

**Migration Notes:**
- Critical for paste functionality
- No API changes expected
- Lock key sync essential for macOS integration

#### src/hardware/input/mouse.cpp - Mouse

**Purpose:** Track mouse activation and movement

**Location (Old):** `src/hardware/mouse.cpp`
**Location (New):** `src/hardware/input/mouse.cpp` ⚠️ **PATH CHANGED**

**Modifications:**

```cpp
// Mouse activation tracking
void MOUSE_SetActive(bool active) {
    mouse_active = active;

    // BOXER MODIFICATION: Notify Boxer of mouse state
    boxer_setMouseActive(active);
}

// Mouse movement
void MOUSE_UpdatePosition(int x, int y) {
    // ... update internal state ...

    // BOXER MODIFICATION: Notify Boxer of movement
    boxer_mouseMovedToPoint(x, y);
}
```

**Callbacks:**
```cpp
void boxer_setMouseActive(bool active);
void boxer_mouseMovedToPoint(int x, int y);
```

**Migration Notes:**
- Simple path change
- Update include: `"hardware/input/mouse.h"`

#### src/hardware/input/joystick.cpp - Joystick

**Purpose:** Track joystick usage

**Location (Old):** `src/hardware/joystick.cpp`
**Location (New):** `src/hardware/input/joystick.cpp` ⚠️ **PATH CHANGED**

**Modifications:**

```cpp
// Lines 269, 275: Joystick activation
void JOYSTICK_Enable(uint8_t which, bool enabled) {
    joystick_enabled[which] = enabled;

    // BOXER MODIFICATION: Notify Boxer
    boxer_setJoystickActive(which, enabled);
}
```

**Callbacks:**
```cpp
void boxer_setJoystickActive(uint8_t joystick_num, bool active);
```

**Migration Notes:**
- Simple path change
- Update include: `"hardware/input/joystick.h"`

---

### 6. Audio and MIDI

#### src/audio/mixer.cpp - Audio Mixer

**Purpose:** Volume control through Boxer

**Location (Old):** `src/hardware/mixer.cpp`
**Location (New):** `src/audio/mixer.cpp` ⚠️ **PATH CHANGED**

**Modifications:**

```cpp
// Lines 183-184: Master volume getter
float MIXER_GetMasterVolume() {
    // BOXER MODIFICATION: Get volume from Boxer
    return boxer_masterVolume();
}

// Line 803: Volume display
void MIXER_DisplayVolume() {
    float volume = boxer_masterVolume();
    // ... display ...
}
```

**Callbacks:**
```cpp
float boxer_masterVolume();
```

**Migration Notes:**
- ⚠️ **PATH CHANGED:** Major reorganization
- Update include: `"audio/mixer.h"`
- Check if new audio effects interfere with Boxer's volume control

#### src/midi/midi.cpp - MIDI System

**Purpose:** Route MIDI messages through Boxer

**Location (Old):** `src/midi/midi.cpp`
**Location (New):** `src/midi/midi.cpp` (unchanged)

**Modifications:**

```cpp
// Lines 156, 175: MIDI message routing
void MidiHandler::PlayMsg(uint8_t* msg) {
    // BOXER MODIFICATION: Route through Boxer
    boxer_sendMIDIMessage(msg);
}

// Line 218: SysEx routing
void MidiHandler::PlaySysex(uint8_t* sysex, size_t len) {
    // BOXER MODIFICATION: Route through Boxer
    boxer_sendMIDISysex(sysex, len);
}

// Device initialization
void MIDI_Init(Section* sec) {
    // ... setup ...

    // BOXER MODIFICATION: Notify Boxer of MIDI device
    boxer_MIDIDeviceDidInitialize(device_name);
}
```

**Callbacks:**
```cpp
void boxer_sendMIDIMessage(uint8_t* msg);
void boxer_sendMIDISysex(uint8_t* sysex, size_t len);
void boxer_MIDIDeviceDidInitialize(const char* device_name);
```

**Migration Notes:**
- Check new SoundCanvas device integration
- Verify CoreMIDI compatibility on macOS
- Test with FluidSynth 2.5.0

---

### 7. Parallel Port / Printer

#### src/hardware/parport/printer_redir.cpp - Printer Redirection

**Purpose:** Virtual printer through Boxer

**Location (Old):** `src/hardware/parport/printer_redir.cpp`
**Location (New):** ❌ **DOES NOT EXIST** - parport removed

**Original Modifications:**

```cpp
// Line 25: Include BXCoalface
#import "BXCoalface.h"

// Lines 31, 55-71: All I/O redirected to Boxer
Bitu CPrinterRedir::Read_PR() {
    return boxer_PRINTER_readdata(0, 1);
}

bool CPrinterRedir::Putchar(Bit8u val) {
    Write_CON(0xD4);
    Write_PR(val);
    Write_CON(0xD5);
    Write_CON(0xD4);
    Read_SR();
    return true;
}

void CPrinterRedir::Write_PR(Bitu val) {
    boxer_PRINTER_writedata(0, val, 1);
}

Bitu CPrinterRedir::Read_SR() {
    return boxer_PRINTER_readstatus(0, 1);
}

void CPrinterRedir::Write_CON(Bitu val) {
    boxer_PRINTER_writecontrol(0, val, 1);
}

bool CPrinterRedir::IsInited() {
    return boxer_PRINTER_isInited(0);
}
```

**Callbacks:**
```cpp
Bitu boxer_PRINTER_readdata(Bitu port, Bitu iolen);
void boxer_PRINTER_writedata(Bitu port, Bitu val, Bitu iolen);
Bitu boxer_PRINTER_readstatus(Bitu port, Bitu iolen);
void boxer_PRINTER_writecontrol(Bitu port, Bitu val, Bitu iolen);
bool boxer_PRINTER_isInited(Bitu port);
```

**Migration Options:**

**Option 1: Port old parport code forward**
```bash
# Copy entire parport directory
mkdir src/hardware/parport
cp old-dosbox/src/hardware/parport/*.{cpp,h} src/hardware/parport/

# Update includes
# Update build system
```

**Option 2: Stub implementation (if printer unused)**
```cpp
// In BXCoalface.mm
Bitu boxer_PRINTER_readdata(Bitu port, Bitu iolen) {
    return 0xFF;  // No data
}

void boxer_PRINTER_writedata(Bitu port, Bitu val, Bitu iolen) {
    // Silently discard
}

// ... other stubs ...
```

**Option 3: Serial port redirection**
```cpp
// Redirect LPT to serial port
// More complex but uses existing DOSBox infrastructure
```

**Migration Notes:**
- ⚠️ **BREAKING:** Requires decision on printer support
- LPT DAC exists for audio: `src/hardware/audio/lpt_dac.cpp`
- Full printer emulation was in `printer.cpp` (not used by Boxer)

---

### 8. Localization

#### src/misc/messages.cpp - Message System

**Purpose:** Use Boxer's localized strings

**Location (Old):** `src/misc/messages.cpp`
**Location (New):** `src/misc/messages.cpp` (unchanged)

**Modifications:**

```cpp
// Line 127: Message lookup override
const char* MSG_Get(const char* key) {
    // BOXER MODIFICATION: Try Boxer's localization first
    const char* localized = boxer_localizedStringForKey(key);
    if (localized != nullptr) {
        return localized;
    }

    // Fall back to DOSBox default
    return GetDefaultMessage(key);
}
```

**Callbacks:**
```cpp
const char* boxer_localizedStringForKey(const char* key);
```

**Returns:** Localized string or `nullptr` if not found

**Migration Notes:**
- Simple and stable
- No changes expected

---

## BXCoalface.h - Function Replacement Macros

### Purpose

Replace DOSBox functions with Boxer equivalents via preprocessor

### Mechanism

```cpp
// BXCoalface.h
#define GFX_Events         boxer_processEvents
#define GFX_StartUpdate    boxer_startFrame
#define GFX_EndUpdate      boxer_finishFrame
#define Mouse_AutoLock     boxer_setMouseActive
#define GFX_SetTitle       boxer_handleDOSBoxTitleChange
```

### Migration Notes

- Powerful but can hide what's really happening
- Consider replacing with explicit function calls for clarity
- Ensure all macro replacements still match new DOSBox API

### Recommended Improvement

Instead of macros, use inline wrapper functions:
```cpp
// More explicit and debuggable
inline void GFX_Events() {
    boxer_processEvents();
}
```

---

## Callback Function Signatures

### Complete List

#### Rendering (10 functions)

```cpp
uint8_t boxer_prepareForFrameSize(int width, int height,
                                    Fraction aspect_ratio,
                                    uint8_t flags,
                                    VideoMode& mode,
                                    GFX_Callback_t callback);

bool boxer_startFrame(uint8_t** pixels_out, int* pitch_out);
void boxer_finishFrame();

uint8_t boxer_idealOutputMode();
uint32_t boxer_getRGBPaletteEntry(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

bool boxer_setShader(const char* shader_name);

uint8_t boxer_herculesTintMode();
void boxer_setHerculesTintMode(uint8_t mode);

float boxer_CGACompositeHueOffset();
void boxer_setCGACompositeHueOffset(float offset);

double boxer_GetDisplayRefreshRate();
```

#### Shell (14 functions)

```cpp
void boxer_shellWillStart();
void boxer_shellDidFinish();
void boxer_shellWillStartAutoexec();
void boxer_didReturnToShell();

bool boxer_shellShouldRunCommand(const char* command);

void boxer_shellWillReadCommandInputFromHandle(uint16_t handle);
void boxer_shellDidReadCommandInputFromHandle(uint16_t handle);

void boxer_handleShellCommandInput(char* buffer, size_t max_len);

bool boxer_shellShouldContinue();

bool boxer_hasPendingCommandsForShell();
bool boxer_shellShouldDisplayStartupMessages();

void boxer_shellWillExecuteFileAtDOSPath(const char* path);
void boxer_shellWillBeginBatchFile(const char* filename);
void boxer_shellDidEndBatchFile(const char* filename);
```

#### Drive/File Handling (17 functions)

```cpp
bool boxer_shouldMountPath(const char* path);
void boxer_driveDidMount(char drive, const char* path, uint8_t type);
void boxer_driveDidUnmount(char drive);

bool boxer_shouldShowFileWithName(const char* filename);
bool boxer_shouldAllowWriteAccessToPath(const char* path);

void boxer_didCreateLocalFile(const char* path);
void boxer_didRemoveLocalFile(const char* path);

FILE* boxer_openLocalFile(const char* path, const char* mode);
int boxer_removeLocalFile(const char* path);
int boxer_moveLocalFile(const char* old_path, const char* new_path);
int boxer_createLocalDir(const char* path);

void boxer_closeLocalFile(FILE* file);
size_t boxer_readFromLocalFile(void* ptr, size_t size, size_t nmemb, FILE* file);
size_t boxer_writeToLocalFile(const void* ptr, size_t size, size_t nmemb, FILE* file);
int boxer_seekInLocalFile(FILE* file, long offset, int whence);
long boxer_tellInLocalFile(FILE* file);
```

#### Input (12 functions)

```cpp
void boxer_setJoystickActive(uint8_t joystick_num, bool active);

void boxer_setMouseActive(bool active);
void boxer_mouseMovedToPoint(int x, int y);

uint32_t boxer_keyboardBufferRemaining();
void boxer_keyboardLayoutLoaded(const char* layout_name);

void boxer_setCapsLockActive(bool active);
void boxer_setNumLockActive(bool active);
void boxer_setScrollLockActive(bool active);

bool boxer_continueListeningForKeyEvents();

uint32_t boxer_numKeyCodesInPasteBuffer();
uint16_t boxer_getNextKeyCodeInPasteBuffer();
void boxer_keyCodeWasSentToKeyboardBuffer(uint16_t keycode);
```

#### Printer (6 functions)

```cpp
Bitu boxer_PRINTER_readdata(Bitu port, Bitu iolen);
void boxer_PRINTER_writedata(Bitu port, Bitu val, Bitu iolen);
Bitu boxer_PRINTER_readstatus(Bitu port, Bitu iolen);
void boxer_PRINTER_writecontrol(Bitu port, Bitu val, Bitu iolen);
bool boxer_PRINTER_isInited(Bitu port);
const char* boxer_PRINTER_getDeviceName(Bitu port);
```

#### Runloop (4 functions)

```cpp
bool boxer_runLoopShouldContinue();
void boxer_runLoopWillStartWithContextInfo(void* context);
void boxer_runLoopDidFinishWithContextInfo(void* context);
void boxer_processEvents();
```

#### Audio/MIDI (6 functions)

```cpp
float boxer_masterVolume();

void boxer_sendMIDIMessage(uint8_t* msg);
void boxer_sendMIDISysex(uint8_t* sysex, size_t len);
void boxer_MIDIDeviceDidInitialize(const char* device_name);

void boxer_willInitializeAudioSystem();
void boxer_didInitializeAudioSystem();
```

#### Localization & Logging (5 functions)

```cpp
const char* boxer_localizedStringForKey(const char* key);

void boxer_log(const char* message);
void boxer_logWithLevel(int level, const char* message);

[[noreturn]] void boxer_die(const char* function, const char* file,
                             int line, const char* format, ...);
```

#### Miscellaneous (5 functions)

```cpp
void boxer_handleDOSBoxTitleChange(const char* title);
const char* boxer_configPath();
bool boxer_shouldShowCursor();
void boxer_waitForFramebufferSwap();
void boxer_renderFrame();
```

### Total: 99 callback functions

---

## Build System Integration

### Compiler Flags Required

```bash
# C++20 required
-std=c++20

# Objective-C++ for .mm files
-x objective-c++

# Include paths
-I$(SRCROOT)                     # For BXCoalface.h
-I$(SRCROOT)/DOSBox-Staging/src  # For DOSBox headers

# Frameworks
-framework CoreAudio
-framework CoreMIDI
-framework CoreFoundation
-framework CoreServices
-framework IOKit

# Feature flags
-DBOXER_INTEGRATION
```

### Meson Configuration

```meson
# Add Boxer option
option('boxer_integration', type: 'boolean', value: false,
       description: 'Enable Boxer integration hooks')

# In src/meson.build
if get_option('boxer_integration')
    dosbox_sources += [
        'BXCoalface.mm',
        'BXCoalfaceAudio.mm',
    ]

    add_project_arguments('-DBOXER_INTEGRATION', language: 'cpp')
    add_project_link_arguments(
        '-framework', 'CoreFoundation',
        language: 'cpp'
    )
endif
```

### CMake Configuration

```cmake
option(BOXER_INTEGRATION "Enable Boxer integration" OFF)

if(BOXER_INTEGRATION)
    target_sources(dosbox PRIVATE
        src/BXCoalface.mm
        src/BXCoalfaceAudio.mm
    )

    target_compile_definitions(dosbox PRIVATE BOXER_INTEGRATION)

    if(APPLE)
        target_link_libraries(dosbox PRIVATE
            "-framework CoreFoundation"
            "-framework CoreAudio"
            "-framework CoreMIDI"
        )
    endif()
endif()
```

---

## Testing Checklist

### Per-Category Testing

#### Rendering
- [ ] Frame buffer updates correctly
- [ ] Aspect ratio preserved
- [ ] Mode changes (text/graphics)
- [ ] CGA composite mode
- [ ] Hercules tint mode
- [ ] Palette changes

#### Shell
- [ ] Command interception works
- [ ] Batch files tracked
- [ ] Autoexec execution monitored
- [ ] Program execution tracked
- [ ] Input modification functional

#### File System
- [ ] Write protection enforced
- [ ] File creation tracked
- [ ] File deletion tracked
- [ ] Mount restrictions honored
- [ ] System files hidden (.DS_Store)

#### Input
- [ ] Keyboard paste works
- [ ] Caps Lock sync
- [ ] Num Lock sync
- [ ] Scroll Lock sync
- [ ] Mouse activation tracked
- [ ] Mouse movement tracked
- [ ] Joystick detection works

#### Audio/MIDI
- [ ] Volume control functional
- [ ] MIDI messages routed correctly
- [ ] SysEx messages work
- [ ] Multiple MIDI devices supported

#### Printer
- [ ] Printer I/O works (if implemented)
- [ ] OR gracefully disabled (if not)

#### Localization
- [ ] Boxer strings displayed
- [ ] Falls back to English if needed

---

## Migration Priority

### Phase 1: Critical Path
1. Main runloop control
2. Rendering system
3. Shell integration
4. File system protection

### Phase 2: Essential Features
1. Input handling
2. Audio/MIDI
3. Localization

### Phase 3: Optional
1. Printer support (if needed)
2. Advanced features
3. Optimizations

---

## Document Maintenance

This document should be updated:
- When callback signatures change
- When new integration points added
- When DOSBox Staging versions change
- When migration issues discovered

**Last Updated:** 2025-11-14
**DOSBox Version:** 0.83.0-alpha (target)
**Boxer Branch:** dosbox-boxer-upgrade-boxerside
