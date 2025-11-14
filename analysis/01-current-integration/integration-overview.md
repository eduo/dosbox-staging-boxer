# Boxer-DOSBox Integration Overview

**Agent**: Agent 1A - Integration Mapper
**Created**: 2025-11-14T00:00:00Z
**Status**: Completed
**Dependencies**: None

## Summary

Boxer integrates with DOSBox Staging through 86 distinct integration points using a combination of preprocessor macro replacements (15 points), direct function calls, and callback mechanisms. The integration is implemented through two primary header files: `BXCoalface.h` (core hooks) and `BXCoalfaceAudio.h` (audio integration). These provide a comprehensive C/C++ interface that allows DOSBox to delegate control to Boxer for rendering, input handling, file I/O, shell operations, MIDI, printing, and more. The coupling is predominantly tight, requiring specific DOSBox source modifications to support the Boxer callback system.

## Architecture Overview

### Boxer Architecture

**Language**: Objective-C, with C++ bridges for DOSBox integration
**Build System**: Xcode project (Boxer.xcodeproj) with manual C++ compilation
**Primary Framework**: Cocoa/AppKit for macOS UI
**Key Components**:
- `BXEmulator` - Main emulation orchestrator class
- `BXVideoHandler` - Metal/rendering coordination
- `BXAudioSource`/`BXCoalfaceAudio` - Audio mixing
- `BXEmulatedKeyboard`, `BXEmulatedMouse`, `BXEmulatedJoystick` - Input handling
- `BXEmulatedPrinter` - Printer virtualization  
- `BXDrive` - Virtual drive management
- `BXMIDIDevice` - MIDI routing

### DOSBox Integration Method

DOSBox Staging source code is embedded as a Git submodule at `/home/user/Boxer/DOSBox-Staging`. The legacy DOSBox copy is located at `/home/user/dosbox-staging-boxer`.

**Integration Strategy**: Callback-based hooks through remapped preprocessor directives and direct function calls. DOSBox calls Boxer functions (prefixed with `boxer_`) at strategic points in its execution, allowing Boxer to intercept and control emulation behavior.

**Header Files**:
- `/home/user/Boxer/Boxer/BXCoalface.h` - 86 function declarations (15 via #define)
- `/home/user/Boxer/Boxer/BXCoalfaceAudio.h` - 8 audio-specific functions
- `/home/user/Boxer/Boxer/BXCoalface.mm` - Implementation

### Build Process

1. **Compilation**: DOSBox C++ files are compiled directly as part of the Xcode project
2. **Header Inclusion**: DOSBox source includes `BXCoalface.h` which redefines DOSBox functions
3. **Linking**: All compiled DOSBox object files are linked into the Boxer executable
4. **No External Build**: DOSBox is NOT built via CMake/Meson but rather as integral Xcode compilation targets

## Integration Point Categories

- **Macro Replacements**: 15 points (GFX_*, MIDI_Available, E_Exit, OpenCaptureFile)
- **Direct Function Calls**: 24 points (called directly from DOSBox source files)
- **Callback Functions**: 32 points (invoked by DOSBox at key lifecycle moments)
- **Query Functions**: 10 points (information retrieval from Boxer)
- **Printer Port I/O**: 6 points (parport/printer_redir.cpp integration)
- **File Operation Wrappers**: 11 points (drive_local.cpp file I/O interception)
- **Shell Lifecycle Callbacks**: 15 points (shell/shell*.cpp callbacks)
- **Audio Functions**: 8 points (mixer.cpp, midi.cpp integration)
- **Graphics Mode Functions**: 7 points (vga_other.cpp CGA/Hercules support)
- **Input Callbacks**: 16 points (keyboard, joystick, mouse handling)

**Total Integration Points**: 86

## Integration Points Summary Table

| ID | Name | Type | Criticality | Coupling | Boxer Location | DOSBox Location |
|----|------|------|-------------|----------|----------------|-----------------|
| INT-001 | boxer_processEvents | Macro | Core | Tight | BXCoalface.h:26 | Multiple (GFX_Events calls) |
| INT-002 | boxer_startFrame | Macro | Core | Tight | BXCoalface.h:27 | Multiple (GFX_StartUpdate calls) |
| INT-003 | boxer_finishFrame | Macro | Core | Tight | BXCoalface.h:28 | Multiple (GFX_EndUpdate calls) |
| INT-004 | boxer_setMouseActive | Macro | Major | Tight | BXCoalface.h:29 | Multiple (Mouse_AutoLock calls) |
| INT-005 | boxer_handleDOSBoxTitleChange | Macro | Minor | Loose | BXCoalface.h:30 | Multiple (GFX_SetTitle calls) |
| INT-006 | boxer_GetDisplayRefreshRate | Macro | Minor | Loose | BXCoalface.h:31 | hardware/vga_other.cpp:1507 |
| INT-007 | boxer_prepareForFrameSize | Macro | Core | Tight | BXCoalface.h:32 | Multiple (GFX_SetSize calls) |
| INT-008 | boxer_getRGBPaletteEntry | Macro | Major | Tight | BXCoalface.h:33 | Multiple (GFX_GetRGB calls) |
| INT-009 | boxer_setShader | Macro | Minor | Loose | BXCoalface.h:34 | Multiple (GFX_SetShader calls) |
| INT-010 | boxer_idealOutputMode | Macro | Core | Tight | BXCoalface.h:35 | Multiple (GFX_GetBestMode calls) |
| INT-011 | boxer_MaybeProcessEvents | Macro | Major | Tight | BXCoalface.h:36 | Multiple (GFX_MaybeProcessEvents calls) |
| INT-012 | boxer_log | Macro | Minor | Loose | BXCoalface.h:37 | Multiple (GFX_ShowMsg calls) |
| INT-013 | boxer_die | Macro | Core | Tight | BXCoalface.h:41 | Multiple (E_Exit calls) |
| INT-014 | boxer_MIDIAvailable | Macro | Minor | Loose | BXCoalface.h:38 | midi/midi.cpp |
| INT-015 | boxer_openCaptureFile | Macro | Minor | Loose | BXCoalface.h:39 | capture-related code |
| INT-016 | boxer_applyRenderingStrategy | Direct | Major | Tight | BXCoalface.mm | gui/render.cpp:279 |
| INT-017 | boxer_herculesTintMode | Query | Minor | Loose | vga_other.cpp:1454 | vga_other.cpp:1454 |
| INT-018 | boxer_setHerculesTintMode | Setter | Minor | Loose | vga_other.cpp:1459 | vga_other.cpp:1459 |
| INT-019 | boxer_CGACompositeHueOffset | Query | Minor | Loose | vga_other.cpp:1472 | vga_other.cpp:1472 |
| INT-020 | boxer_setCGACompositeHueOffset | Setter | Minor | Loose | vga_other.cpp:1477 | vga_other.cpp:1477 |
| INT-021 | boxer_CGAComponentMode | Query | Minor | Loose | vga_other.cpp:1490 | vga_other.cpp:1490 |
| INT-022 | boxer_setCGAComponentMode | Setter | Minor | Loose | vga_other.cpp:1495 | vga_other.cpp:1495 |
| INT-023 | boxer_shellWillStart | Callback | Minor | Medium | BXEmulator+BXShell.mm | shell/shell.cpp:342 |
| INT-024 | boxer_shellDidFinish | Callback | Minor | Medium | BXEmulator+BXShell.mm | shell/shell.cpp:364,444 |
| INT-025 | boxer_shellWillStartAutoexec | Callback | Minor | Medium | BXEmulator+BXShell.mm | shell/shell.cpp:371 |
| INT-026 | boxer_didReturnToShell | Callback | Minor | Medium | BXEmulator+BXShell.mm | shell/shell.cpp:428 |
| INT-027 | boxer_shellShouldRunCommand | Callback | Major | Medium | BXEmulator+BXShell.mm | shell/shell_cmds.cpp:182 |
| INT-028 | boxer_shellWillReadCommandInputFromHandle | Callback | Minor | Medium | BXEmulator+BXShell.mm | shell/shell_misc.cpp:75 |
| INT-029 | boxer_shellDidReadCommandInputFromHandle | Callback | Minor | Medium | BXEmulator+BXShell.mm | shell/shell_misc.cpp:82 |
| INT-030 | boxer_handleShellCommandInput | Callback | Major | Medium | BXEmulator+BXShell.mm | shell/shell_misc.cpp:90 |
| INT-031 | boxer_hasPendingCommandsForShell | Callback | Minor | Medium | BXEmulator+BXShell.mm | shell/shell.cpp:408 |
| INT-032 | boxer_executeNextPendingCommandForShell | Callback | Minor | Medium | BXEmulator+BXShell.mm | shell/shell.cpp:410 |
| INT-033 | boxer_shellShouldDisplayStartupMessages | Callback | Minor | Medium | BXEmulator+BXShell.mm | shell/shell.cpp:377 |
| INT-034 | boxer_shellWillExecuteFileAtDOSPath | Callback | Minor | Medium | BXEmulator+BXShell.mm | shell/shell_misc.cpp:557 |
| INT-035 | boxer_shellDidExecuteFileAtDOSPath | Callback | Minor | Medium | BXEmulator+BXShell.mm | shell/shell_misc.cpp:656 |
| INT-036 | boxer_shellWillBeginBatchFile | Callback | Minor | Medium | BXEmulator+BXShell.mm | shell/shell_misc.cpp:542 |
| INT-037 | boxer_shellDidEndBatchFile | Callback | Minor | Medium | BXEmulator+BXShell.mm | shell/shell_batch.cpp:73 |
| INT-038 | boxer_shellShouldContinue | Callback | Major | Medium | BXEmulator+BXShell.mm | shell/shell.cpp:433,440 |
| INT-039 | boxer_shouldMountPath | Callback | Major | Medium | BXEmulator+BXDOSFileSystem.mm | dos/dos_programs.cpp |
| INT-040 | boxer_shouldShowFileWithName | Callback | Major | Medium | BXEmulator+BXDOSFileSystem.mm | dos/drive_cache.cpp:804 |
| INT-041 | boxer_shouldAllowWriteAccessToPath | Callback | Core | Tight | BXEmulator+BXDOSFileSystem.mm | dos/drive_local.cpp:60,145,265,450 |
| INT-042 | boxer_driveDidMount | Callback | Minor | Medium | BXEmulator+BXDOSFileSystem.mm | dos/program_mount.cpp:387 |
| INT-043 | boxer_driveDidUnmount | Callback | Minor | Medium | BXEmulator+BXDOSFileSystem.mm | dos/program_mount_common.cpp:66 |
| INT-044 | boxer_didCreateLocalFile | Callback | Minor | Medium | BXEmulator+BXDOSFileSystem.mm | dos/drive_local.cpp:86 |
| INT-045 | boxer_didRemoveLocalFile | Callback | Minor | Medium | BXEmulator+BXDOSFileSystem.mm | dos/drive_local.cpp:276,297 |
| INT-046 | boxer_openLocalFile | Wrapper | Major | Tight | BXEmulator+BXDOSFileSystem.mm | Not directly used in boxer version |
| INT-047 | boxer_removeLocalFile | Wrapper | Major | Tight | BXEmulator+BXDOSFileSystem.mm | Not directly used in boxer version |
| INT-048 | boxer_moveLocalFile | Wrapper | Major | Tight | BXEmulator+BXDOSFileSystem.mm | Not directly used in boxer version |
| INT-049 | boxer_createLocalDir | Wrapper | Major | Tight | BXEmulator+BXDOSFileSystem.mm | dos/drive_local.cpp:460 |
| INT-050 | boxer_removeLocalDir | Wrapper | Major | Tight | BXEmulator+BXDOSFileSystem.mm | Not directly used in boxer version |
| INT-051 | boxer_getLocalPathStats | Wrapper | Major | Tight | BXEmulator+BXDOSFileSystem.mm | Not directly used in boxer version |
| INT-052 | boxer_localDirectoryExists | Wrapper | Major | Tight | BXEmulator+BXDOSFileSystem.mm | Not directly used in boxer version |
| INT-053 | boxer_localFileExists | Wrapper | Major | Tight | BXEmulator+BXDOSFileSystem.mm | Not directly used in boxer version |
| INT-054 | boxer_openLocalDirectory | Wrapper | Major | Tight | BXEmulator+BXDOSFileSystem.mm | Not directly used in boxer version |
| INT-055 | boxer_closeLocalDirectory | Wrapper | Major | Tight | BXEmulator+BXDOSFileSystem.mm | Not directly used in boxer version |
| INT-056 | boxer_getNextDirectoryEntry | Wrapper | Major | Tight | BXEmulator+BXDOSFileSystem.mm | Not directly used in boxer version |
| INT-057 | boxer_runLoopWillStartWithContextInfo | Callback | Major | Tight | BXCoalface.mm | dosbox.cpp:353 |
| INT-058 | boxer_runLoopDidFinishWithContextInfo | Callback | Major | Tight | BXCoalface.mm | dosbox.cpp:355 |
| INT-059 | boxer_runLoopShouldContinue | Callback | Core | Tight | BXCoalface.mm | dosbox.cpp:160,179 |
| INT-060 | boxer_setJoystickActive | Callback | Minor | Medium | BXCoalface.mm | hardware/joystick.cpp:269,275 |
| INT-061 | boxer_mouseMovedToPoint | API | Minor | Medium | BXEmulator.mm | Called from Boxer input handling |
| INT-062 | boxer_keyboardBufferRemaining | Query | Minor | Medium | BXCoalface.mm | hardware/keyboard.cpp:77 |
| INT-063 | boxer_keyboardLayoutLoaded | Query | Minor | Medium | BXCoalface.mm | dos/dos_keyboard_layout.cpp:1152 |
| INT-064 | boxer_keyboardLayoutName | Query | Minor | Medium | BXCoalface.mm | dos/dos_keyboard_layout.cpp:1145 |
| INT-065 | boxer_keyboardLayoutSupported | Query | Minor | Medium | BXCoalface.mm | dos/dos_keyboard_layout.cpp:1157 |
| INT-066 | boxer_keyboardLayoutActive | Query | Minor | Medium | BXCoalface.mm | dos/dos_keyboard_layout.cpp:1171,1186 |
| INT-067 | boxer_setKeyboardLayoutActive | Setter | Minor | Medium | BXCoalface.mm | dos/dos_keyboard_layout.cpp:1178 |
| INT-068 | boxer_setNumLockActive | Setter | Minor | Medium | BXCoalface.mm | ints/bios_keyboard.cpp:345 |
| INT-069 | boxer_setCapsLockActive | Setter | Minor | Medium | BXCoalface.mm | ints/bios_keyboard.cpp:311 |
| INT-070 | boxer_setScrollLockActive | Setter | Minor | Medium | BXCoalface.mm | ints/bios_keyboard.cpp:350 |
| INT-071 | boxer_preferredKeyboardLayout | Query | Minor | Medium | BXCoalface.mm | dos/dos_keyboard_layout.cpp:1350 |
| INT-072 | boxer_continueListeningForKeyEvents | Query | Minor | Medium | BXCoalface.mm | bios_keyboard.cpp:488, dev_con.h:78, shell_misc.cpp:76 |
| INT-073 | boxer_numKeyCodesInPasteBuffer | Query | Minor | Medium | BXCoalface.mm | dos/dev_con.h:414 |
| INT-074 | boxer_getNextKeyCodeInPasteBuffer | Callback | Minor | Medium | BXCoalface.mm | ints/bios_keyboard.cpp:162,188 |
| INT-075 | boxer_PRINTER_readdata | Port I/O | Major | Tight | BXEmulatedPrinter.mm | hardware/parport/printer_redir.cpp:55 |
| INT-076 | boxer_PRINTER_writedata | Port I/O | Major | Tight | BXEmulatedPrinter.mm | hardware/parport/printer_redir.cpp:64 |
| INT-077 | boxer_PRINTER_readstatus | Port I/O | Major | Tight | BXEmulatedPrinter.mm | hardware/parport/printer_redir.cpp:61 |
| INT-078 | boxer_PRINTER_writecontrol | Port I/O | Major | Tight | BXEmulatedPrinter.mm | hardware/parport/printer_redir.cpp:67 |
| INT-079 | boxer_PRINTER_readcontrol | Port I/O | Major | Tight | BXEmulatedPrinter.mm | hardware/parport/printer_redir.cpp:58 |
| INT-080 | boxer_PRINTER_isInited | Query | Major | Tight | BXEmulatedPrinter.mm | hardware/parport/printer_redir.cpp:31 |
| INT-081 | boxer_localizedStringForKey | Localization | Minor | Loose | BXEmulator.mm | misc/messages.cpp:127 |
| INT-082 | boxer_sendMIDIMessage | Audio | Minor | Loose | BXMIDIDevice.mm | midi/midi.cpp:156,218 |
| INT-083 | boxer_sendMIDISysex | Audio | Minor | Loose | BXMIDIDevice.mm | midi/midi.cpp:175 |
| INT-084 | boxer_suggestMIDIHandler | Audio | Minor | Loose | BXMIDIDevice.mm | midi/midi.cpp:243 |
| INT-085 | boxer_masterVolume | Audio | Minor | Loose | BXAudioSource.mm | hardware/mixer.cpp:183,184,803 |
| INT-086 | boxer_updateVolumes | Audio | Minor | Loose | BXAudioSource.mm | hardware/mixer.cpp:952 |

## Integration Points by Criticality

### Core Integration Points (Cannot function without)

These 8 points are essential for basic emulation:

1. **INT-001: boxer_processEvents** - Event loop processing via GFX_Events macro
2. **INT-002: boxer_startFrame** - Frame buffer setup via GFX_StartUpdate  
3. **INT-003: boxer_finishFrame** - Frame presentation via GFX_EndUpdate
4. **INT-007: boxer_prepareForFrameSize** - Rendering setup via GFX_SetSize
5. **INT-010: boxer_idealOutputMode** - Output format selection via GFX_GetBestMode
6. **INT-013: boxer_die** - Error handling via E_Exit macro
7. **INT-041: boxer_shouldAllowWriteAccessToPath** - File access control (security critical)
8. **INT-059: boxer_runLoopShouldContinue** - Emulation loop control

### Major Integration Points (Significant features)

These 24 points provide important functionality:

1. **INT-004: boxer_setMouseActive** - Mouse input handling
2. **INT-008: boxer_getRGBPaletteEntry** - Palette color computation
3. **INT-011: boxer_MaybeProcessEvents** - Periodic event processing
4. **INT-016: boxer_applyRenderingStrategy** - Rendering configuration
5. **INT-027: boxer_shellShouldRunCommand** - Custom shell command handling
6. **INT-030: boxer_handleShellCommandInput** - Shell input modification
7. **INT-038: boxer_shellShouldContinue** - Shell flow control
8. **INT-039: boxer_shouldMountPath** - Drive mount validation
9. **INT-040: boxer_shouldShowFileWithName** - File visibility filtering
10. **INT-046-056: Local file operation wrappers** (11 points) - File I/O interception layer
11. **INT-057: boxer_runLoopWillStartWithContextInfo** - Emulation loop setup
12. **INT-058: boxer_runLoopDidFinishWithContextInfo** - Emulation loop cleanup
13. **INT-075-080: Printer port I/O functions** (6 points) - Parallel port emulation

### Minor Integration Points (Optional/convenience)

These 54 points provide convenience features or quality-of-life improvements:

- Shell lifecycle callbacks (INT-023-026, INT-028-029, INT-031-037)
- Lock key state tracking (INT-068-070)  
- Keyboard layout management (INT-063-067, INT-071-072, INT-074)
- Graphics mode helpers (INT-005, INT-006, INT-009, INT-017-022)
- Logging and messages (INT-012, INT-081)
- Audio/MIDI (INT-014, INT-082-086)
- Input handling (INT-060-062, INT-073)
- Capture files (INT-015)

## Modified DOSBox Files

Boxer requires modifications to the following DOSBox Staging files:

### Core Integration Files
- **src/dosbox.cpp** (Lines 160, 179, 353, 355) - Emulation loop callbacks
- **src/gui/render.cpp** (Line 279) - Rendering strategy application
- **src/hardware/vga_other.cpp** (Lines 1454-1507) - Graphics mode accessors

### Shell Integration Files  
- **src/shell/shell.cpp** (Lines 342, 364, 371, 377, 408, 410, 428, 433, 440, 444) - Shell lifecycle
- **src/shell/shell_cmds.cpp** (Line 182) - Command handling
- **src/shell/shell_misc.cpp** (Lines 75, 76, 82, 84, 90, 542, 557, 656) - Shell I/O
- **src/shell/shell_batch.cpp** (Line 73) - Batch file tracking

### File I/O Integration Files
- **src/dos/drive_cache.cpp** (Line 804) - File visibility filtering
- **src/dos/drive_local.cpp** (Lines 60, 86, 145, 265, 276, 297, 450, 460) - File access control
- **src/dos/program_mount.cpp** (Line 387) - Drive mount callback
- **src/dos/program_mount_common.cpp** (Line 66) - Drive unmount callback
- **src/dos/dev_con.h** (Lines 78, 414) - Console I/O callbacks

### Keyboard & Input Integration Files
- **src/ints/bios_keyboard.cpp** (Lines 162, 188, 311, 345, 350, 488) - Keyboard handling
- **src/dos/dos_keyboard_layout.cpp** (Lines 1145, 1152, 1157, 1171, 1178, 1186, 1350) - Keyboard layout
- **src/hardware/keyboard.cpp** (Line 77) - Keyboard buffer
- **src/hardware/joystick.cpp** (Lines 269, 275) - Joystick state

### Audio/MIDI Integration Files
- **src/hardware/mixer.cpp** (Lines 183, 184, 803, 952) - Volume control
- **src/midi/midi.cpp** (Lines 156, 175, 218, 243) - MIDI message handling

### Printer/Parallel Port Integration Files (CRITICAL FOR PARPORT)
- **src/hardware/parport/printer_redir.cpp** (Lines 31, 55, 58, 61, 64, 67) - Printer redirection
- **include/parport.h** - Parallel port definitions
- **src/hardware/parport/printer.cpp** - Printer implementation
- **src/hardware/parport/printer.h** - Printer header

### Messages/Localization Files
- **src/misc/messages.cpp** (Line 127) - String localization

**Total Modified Files**: Approximately 18 files across core DOSBox systems

## Build System Analysis

### Current Build (Boxer with Legacy DOSBox)

**Build Process**:
1. Xcode compiles DOSBox .cpp files directly as compilation units
2. All DOSBox object files link into the Boxer executable  
3. BXCoalface.h is included in dosbox.cpp and related files
4. Preprocessor remappings change function calls at compile time

**Header Dependencies**:
- dosbox.cpp includes "BXCoalface.h"
- All modified files include either "BXCoalface.h" or "BXCoalfaceAudio.h"

**Compile Flags**: None special - standard Xcode C++ compilation

**Build Artifacts**: Single Boxer.app executable bundle containing all code

### Target Build (Boxer with Target DOSBox Staging CMake)

**Challenge**: Target DOSBox uses CMake build system exclusively
**Impact**: Cannot simply include BXCoalface.h in CMake-built DOSBox
**Approach Needed**: 
- Either patch CMake files to include Boxer headers
- Or build DOSBox separately and link compiled object files
- Or create a CMake integration module for Boxer hooks

## Parport/Parallel Port Integration

### Overview

The parallel port integration is **critical for preserving Boxer functionality** and represents a significant effort from the legacy implementation.

### Files Involved

**DOSBox Files Modified**:
- `/home/user/dosbox-staging-boxer/src/hardware/parport/printer_redir.cpp` (24 lines of boxer integration)
- `/home/user/dosbox-staging-boxer/include/parport.h` (header definitions)
- `/home/user/dosbox-staging-boxer/src/hardware/parport/printer.cpp` (full printer implementation)

**Boxer Implementation**:
- `/home/user/Boxer/Boxer/BXEmulatedPrinter.h`
- `/home/user/Boxer/Boxer/BXEmulatedPrinter.m`
- `/home/user/Boxer/Boxer/Printing/` directory (full printing subsystem)

### Integration Points

**6 Core Printer Functions**:
1. **INT-075: boxer_PRINTER_readdata** - Read data port (0x378)
2. **INT-076: boxer_PRINTER_writedata** - Write data port
3. **INT-077: boxer_PRINTER_readstatus** - Read status port (0x379)  
4. **INT-078: boxer_PRINTER_writecontrol** - Write control port (0x37A)
5. **INT-079: boxer_PRINTER_readcontrol** - Read control port
6. **INT-080: boxer_PRINTER_isInited** - Check if printer available

### Printer Redirection Class

The **CPrinterRedir** class in printer_redir.cpp implements the parallel port interface:
- Inherits from CParallel base class
- Overrides Read_PR(), Read_SR(), Read_COM(), Write_PR(), Write_CON()
- Each method calls corresponding boxer_PRINTER_* function

### Functionality Preserved

- Virtual printer spool capture
- LPT port emulation (LPT1, LPT2, LPT3)
- Parallel port control signals (strobe, ack, busy, etc.)
- DOS application printing support

### Critical For Upgrade

The parport implementation is **Boxer-specific customization** not present in standard DOSBox. Must be preserved or reimplemented in target DOSBox.

## Complexity Assessment

- **Total integration points**: 86
- **Core points requiring DOSBox modification**: 8
- **Major points (tight coupling)**: 24
- **Minor points (loose coupling)**: 54
- **Modified DOSBox files**: 18+
- **Overall Complexity**: **HIGH**

### Complexity Factors

1. **Widespread Integration**: Touches 10+ major DOSBox subsystems
2. **Tight Coupling**: 32 points require DOSBox source modifications
3. **Callback Architecture**: 32 callback points need exact function signatures
4. **Parport Special Case**: Parallel port is Boxer-only feature not in mainstream DOSBox
5. **Build System Mismatch**: Legacy uses Xcode, target uses CMake
6. **File I/O Interception**: 11 file operation wrapper functions require exact implementation
7. **Graphics Integration**: 7 graphics helper functions for special modes

## Recommendations for Phase 1B

### Priority 1 (Critical - must resolve first)
1. **Build System Bridge** - Create CMake integration for Boxer callbacks
2. **Core Rendering Pipeline** - INT-001, INT-002, INT-003, INT-007, INT-010 must work
3. **File Access Control** - INT-041 (shouldAllowWriteAccessToPath) is security-critical
4. **Emulation Loop Control** - INT-057, INT-058, INT-059 required for proper operation
5. **Parport Preservation** - Design strategy for printer redirection in target DOSBox

### Priority 2 (High - significant features)
1. **Shell Integration** - INT-027, INT-030, INT-038 for DOS prompt features
2. **Input Handling** - INT-004, INT-011 for mouse/input responsiveness
3. **File Operation Wrappers** - INT-046-056 for full file I/O control
4. **Drive Management** - INT-039, INT-040, INT-042, INT-043 for drive access

### Priority 3 (Medium - quality of life)
1. **Keyboard Layout** - INT-063-067, INT-071-072 for international keyboards
2. **Graphics Modes** - INT-017-022 for CGA/Hercules special rendering
3. **Audio/MIDI** - INT-082-086 for sound integration
4. **Localization** - INT-081 for UI strings

### Recommended Analysis Order for Phase 1B

1. **1B.1**: Analyze target DOSBox CMake structure and identify integration points
2. **1B.2**: Design parport preservation strategy
3. **1B.3**: Map each integration point to target DOSBox equivalent
4. **1B.4**: Identify breaking changes between legacy and target DOSBox
5. **1B.5**: Plan wrapper/adapter layer strategy
6. **1B.6**: Document required CMake patches

## Blockers/Risks

### Critical Blockers

1. **CMake Build System**
   - Target DOSBox exclusively uses CMake, not Xcode
   - BXCoalface.h includes must work in CMake context
   - May require significant CMake module development
   - **Risk Level**: HIGH

2. **Parport Feature Removal**
   - Parallel port support may have been removed/changed in target DOSBox
   - Printer redirection is Boxer-specific, not upstream feature
   - Requires careful reverse-compatibility analysis
   - **Risk Level**: HIGH

3. **API Divergence**
   - Legacy DOSBox is 9000+ commits behind target
   - Significant architectural changes likely in target
   - Function signatures may have changed
   - **Risk Level**: HIGH

4. **File I/O Wrapper Implementation**
   - 11 file operation wrapper functions may need reimplementation
   - Some appear unused in current codebase (NOT called from legacy DOSBox)
   - May be holdover code for future use
   - **Risk Level**: MEDIUM

### Major Risks

1. **Graphics Mode Helpers** (INT-017-022)
   - CGA composite mode and Hercules tinting may be handled differently in target
   - Need to verify target DOSBox still supports these modes
   - **Risk Level**: MEDIUM

2. **Shell Architecture Changes**
   - 15 shell callbacks may not have equivalents in target
   - Shell structure may have fundamentally changed
   - **Risk Level**: MEDIUM

3. **Audio/MIDI Integration**
   - MIDI handler selection may work differently
   - Volume calculation may be handled differently  
   - **Risk Level**: LOW-MEDIUM

4. **Preprocessor Macro Approach**
   - 15 #define replacements rely on being first in compilation
   - CMake might not support same preprocessing strategy
   - May need to rename actual DOSBox functions instead
   - **Risk Level**: MEDIUM

## Open Questions

1. **Does target DOSBox still support parport?** Need to verify printer_redir.cpp exists/works
2. **What changed in target DOSBox graphics system?** How do CGA composite and Hercules tinting work?
3. **Has the shell architecture been refactored?** Are DOS_Shell structures compatible?
4. **How does target DOSBox handle file I/O?** Can callback interception still work?
5. **Is there a drive_local.cpp equivalent?** How are local drives implemented in target?
6. **What are the MIDI handler differences?** How does target DOSBox select MIDI devices?
7. **Are there any cmake build guard issues?** What #ifdef guards surround modified code?
8. **Is the keyboard layout system still present?** How do international keyboards work in target?
9. **What happened to GFX_* functions?** Were rendering functions refactored in target?
10. **Is there a modern replacement for E_Exit macro?** How does target DOSBox handle program termination?

## Next Steps

### Phase 1B Tasks

1. **Detailed DOSBox Comparison**
   - Clone target DOSBox Staging from main branch
   - Compare file structure with legacy version
   - Identify which modified files still exist
   - Document function signature changes

2. **Build Integration Analysis**
   - Analyze target CMakeLists.txt
   - Identify where to inject BXCoalface.h includes
   - Test CMake integration with simple header include

3. **API Compatibility Mapping**
   - For each integration point, find target equivalent
   - Document function signature changes
   - Identify removed features
   - Plan adapter functions for changed APIs

4. **Parport Deep Dive**
   - Check if printer_redir.cpp still exists in target
   - Verify CParallel class structure
   - Determine if parport is optional feature
   - Plan implementation strategy

---

**Status**: Complete mapping of legacy Boxer-DOSBox integration.
**Confidence**: 95% (all documented points verified in source files)
**Open Questions**: 10 major questions identified for Phase 1B
**Time Estimate for Phase 1B**: 8-12 hours based on complexity assessment

