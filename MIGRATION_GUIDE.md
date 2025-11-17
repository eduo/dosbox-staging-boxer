# Boxer DOSBox Staging Migration Guide

**Version:** 1.0
**Date:** 2025-11-14
**Audience:** Developers performing the upgrade

## Overview

This is a practical, step-by-step guide for migrating Boxer from DOSBox Staging 0.78.0 to 0.83.0-alpha. Follow these instructions sequentially, testing at each milestone.

## ⚠️ CRITICAL: Read This First

**Before proceeding with any phases below, you MUST complete the preparation steps in `PREPARATION_AND_CLEANUP.md`.**

The original version of this guide had a critical flaw: it instructed copying files into new DOSBox without first addressing the existing integration with old DOSBox. This would cause conflicts and data loss.

**Required preparation phases (from PREPARATION_AND_CLEANUP.md):**
1. **Phase 0:** Backup current working state
2. **Phase 0.5:** Analyze current integration method
3. **Phase 0.75:** Choose transition strategy (parallel recommended)
4. **Phase 1-prep:** Set up new DOSBox alongside old
5. **Phase 2-prep:** Safely copy integration files

**Only after completing those phases should you proceed with Phase 1 below.**

**The recommended approach is parallel development:**
- Keep old DOSBox integration intact and working
- Create new DOSBox integration alongside it
- Use Xcode build configurations to switch between them
- Gradually migrate with a safety net

See `PREPARATION_AND_CLEANUP.md` for complete details.

---

## Prerequisites

### Development Environment

**Required:**
- macOS 12.0+ (Monterey or later)
- Xcode 14.0+ with Command Line Tools
- Homebrew package manager
- Git

**Verify Environment:**
```bash
# Check macOS version
sw_vers

# Check Xcode
xcodebuild -version  # Should be 14.0+

# Check Command Line Tools
xcode-select --version

# Check Homebrew
brew --version
```

### Install Dependencies

```bash
# Build tools
brew install meson cmake ninja ccache pkg-config python3

# Required libraries
brew install sdl2 sdl2_net opusfile fluidsynth libslirp speexdsp

# Optional but recommended
brew install libpng zlib-ng tracy
```

### Repository Setup

```bash
# Create work directory
mkdir ~/boxer-upgrade
cd ~/boxer-upgrade

# Clone repositories
git clone https://github.com/eduo/Boxer.git
git clone https://github.com/eduo/dosbox-staging.git dosbox-staging-new
git clone https://github.com/eduo/dosbox-staging-boxer.git dosbox-staging-old

# Checkout correct branches
cd Boxer
git checkout dosbox-boxer-upgrade-boxerside

cd ../dosbox-staging-new
git checkout dosbox-boxer-upgrade-dosboxside

cd ../dosbox-staging-old
git checkout main

cd ..
```

---

## Phase 1: Baseline Build (Week 1, Days 1-2)

### Objective

Verify new DOSBox Staging builds cleanly without Boxer integration.

### Step 1.1: Clean Build Test

```bash
cd ~/boxer-upgrade/dosbox-staging-new

# Setup build
meson setup build --buildtype=release

# Compile
meson compile -C build

# Expected output: Successful build
# Binary location: build/dosbox
```

**Checkpoint:**
- [ ] Build completes without errors
- [ ] Binary exists at `build/dosbox`
- [ ] Can run: `./build/dosbox --version`

### Step 1.2: Identify Version

```bash
cd ~/boxer-upgrade/dosbox-staging-new

# Get version info
./build/dosbox --version

# Get commit hash
git log -1 --format="%H %s"

# Document this information
echo "New DOSBox Version:" > ~/boxer-upgrade/VERSION_INFO.txt
./build/dosbox --version >> ~/boxer-upgrade/VERSION_INFO.txt
git log -1 >> ~/boxer-upgrade/VERSION_INFO.txt
```

**Record:**
- Commit hash
- Version number
- Build date

### Step 1.3: Smoke Test

```bash
cd ~/boxer-upgrade/dosbox-staging-new

# Create test directory
mkdir -p ~/dos-test
echo "Hello from DOSBox" > ~/dos-test/test.txt

# Run DOSBox
./build/dosbox

# In DOSBox:
# mount c ~/dos-test
# c:
# dir
# exit
```

**Checkpoint:**
- [ ] DOSBox starts
- [ ] Can mount directory
- [ ] Can list files
- [ ] Can exit cleanly

---

## Phase 2: Include Path Migration (Week 1, Days 3-5)

### Objective

Update all Boxer code for new DOSBox file locations.

### Step 2.1: Create Path Mapping

```bash
cd ~/boxer-upgrade

# Create migration script
cat > update_includes.sh <<'EOF'
#!/bin/bash

# File path updates
declare -A PATH_MAP=(
    ["src/hardware/mixer.cpp"]="src/audio/mixer.cpp"
    ["src/hardware/keyboard.cpp"]="src/hardware/input/keyboard.cpp"
    ["src/hardware/mouse.cpp"]="src/hardware/input/mouse.cpp"
    ["src/hardware/joystick.cpp"]="src/hardware/input/joystick.cpp"
    ["src/gui/render.cpp"]="src/gui/render/render.cpp"
    ["src/hardware/vga_other.cpp"]="src/hardware/video/vga_other.cpp"
    ["src/misc/setup.cpp"]="src/config/setup.cpp"
)

# Include path updates
declare -A INCLUDE_MAP=(
    ['"mixer.h"']='"audio/mixer.h"'
    ['"render.h"']='"gui/render/render.h"'
    ['"keyboard.h"']='"hardware/input/keyboard.h"'
    ['"mouse.h"']='"hardware/input/mouse.h"'
    ['"joystick.h"']='"hardware/input/joystick.h"'
    ['"vga.h"']='"hardware/video/vga.h"'
    ['"setup.h"']='"config/setup.h"'
)

echo "Include path mapping created"
for old in "${!INCLUDE_MAP[@]}"; do
    echo "$old -> ${INCLUDE_MAP[$old]}"
done
EOF

chmod +x update_includes.sh
./update_includes.sh
```

### Step 2.2: Update BXCoalface.h

```bash
cd ~/boxer-upgrade/Boxer

# Backup original
cp Boxer/BXCoalface.h Boxer/BXCoalface.h.backup

# Update includes
# Do this manually or with sed (be careful!)

# Example for one replacement:
sed -i.bak 's/#include "mixer\.h"/#include "audio\/mixer.h"/' Boxer/BXCoalface.h
```

**Manual edits needed in `Boxer/BXCoalface.h`:**

```cpp
// OLD
#include "mixer.h"
#include "render.h"
#include "keyboard.h"
#include "mouse.h"
#include "joystick.h"
#include "vga.h"
#include "setup.h"

// NEW
#include "audio/mixer.h"
#include "gui/render/render.h"
#include "hardware/input/keyboard.h"
#include "hardware/input/mouse.h"
#include "hardware/input/joystick.h"
#include "hardware/video/vga.h"
#include "config/setup.h"
```

### Step 2.3: Update Callback Signatures

Edit `Boxer/BXCoalface.h`:

```cpp
// OLD (0.78.0) rendering callbacks
void boxer_prepareForFrameSize(int width, int height, float aspect,
                                uint8_t flags, void* mode, void* callback);
void boxer_startFrame();
void boxer_finishFrame();

// NEW (0.83.0) rendering callbacks
uint8_t boxer_prepareForFrameSize(int width, int height,
                                    Fraction aspect_ratio,
                                    uint8_t flags,
                                    VideoMode& mode,
                                    GFX_Callback_t callback);
bool boxer_startFrame(uint8_t** pixels_out, int* pitch_out);
void boxer_finishFrame();
```

**Add Fraction helper if needed:**

```cpp
// In BXCoalface.h
struct Fraction {
    int numerator;
    int denominator;
};

// Helper function
inline float FractionToFloat(Fraction f) {
    return static_cast<float>(f.numerator) / static_cast<float>(f.denominator);
}
```

### Step 2.4: Update BXCoalface.mm

Edit `Boxer/BXCoalface.mm`:

```objc
// Update for new signature
uint8_t boxer_prepareForFrameSize(int width, int height,
                                    Fraction aspect_ratio,
                                    uint8_t flags,
                                    VideoMode& mode,
                                    GFX_Callback_t callback) {
    // Convert Fraction to float for Boxer
    float aspect_float = FractionToFloat(aspect_ratio);

    BXEmulator *emulator = [BXEmulator currentEmulator];
    return [emulator _prepareForFrameWidth:width
                                    height:height
                               aspectRatio:aspect_float
                                     flags:flags];
}

// Update startFrame for new signature
bool boxer_startFrame(uint8_t** pixels_out, int* pitch_out) {
    BXEmulator *emulator = [BXEmulator currentEmulator];

    // Get framebuffer from Boxer
    uint8_t* pixels = [emulator _frameBufferPointer];
    int pitch = [emulator _frameBufferPitch];

    if (pixels && pitch > 0) {
        *pixels_out = pixels;
        *pitch_out = pitch;
        return true;
    }

    return false;
}
```

**Checkpoint:**
- [ ] All include paths updated
- [ ] Callback signatures match new API
- [ ] BXCoalface.h compiles without errors (test with: `clang++ -std=c++20 -fsyntax-only BXCoalface.h`)

---

## Phase 3: Copy Integration Files (Week 2, Day 1)

### Objective

Copy Boxer integration files into new DOSBox.

### Step 3.1: Copy BXCoalface Files

```bash
cd ~/boxer-upgrade

# Copy to new DOSBox
cp Boxer/Boxer/BXCoalface.h dosbox-staging-new/src/
cp Boxer/Boxer/BXCoalface.mm dosbox-staging-new/src/
cp Boxer/Boxer/BXCoalfaceAudio.h dosbox-staging-new/src/
cp Boxer/Boxer/BXCoalfaceAudio.mm dosbox-staging-new/src/

# Verify copies
ls -l dosbox-staging-new/src/BX*.{h,mm}
```

### Step 3.2: Inject BXCoalface.h into dosbox.h

```bash
cd ~/boxer-upgrade/dosbox-staging-new

# Edit src/dosbox.h
# Add after all other includes:
```

```cpp
// At end of src/dosbox.h, after all other includes:

#ifdef BOXER_INTEGRATION
#include "BXCoalface.h"
#endif
```

**Manual edit required:** Use your editor to add this to `src/dosbox.h`

### Step 3.3: Update Build System

#### Meson Build

Create `meson_options.txt` addition:

```bash
cd ~/boxer-upgrade/dosbox-staging-new

# Add to meson_options.txt
cat >> meson_options.txt <<'EOF'

option('boxer_integration',
       type: 'boolean',
       value: false,
       description: 'Enable Boxer integration hooks')
EOF
```

Edit `src/meson.build`:

```meson
# Add near top of src/meson.build

if get_option('boxer_integration')
    dosbox_sources += files(
        'BXCoalface.mm',
        'BXCoalfaceAudio.mm',
    )

    add_project_arguments('-DBOXER_INTEGRATION', language: 'cpp')

    if host_machine.system() == 'darwin'
        add_project_link_arguments(
            '-framework', 'CoreFoundation',
            '-framework', 'CoreAudio',
            '-framework', 'CoreMIDI',
            '-framework', 'CoreServices',
            '-framework', 'IOKit',
            language: 'cpp'
        )
    endif
endif
```

### Step 3.4: Test Compilation

```bash
cd ~/boxer-upgrade/dosbox-staging-new

# Clean previous build
rm -rf build

# Setup with Boxer integration
meson setup build -Dboxer_integration=true

# Compile
meson compile -C build 2>&1 | tee compile.log

# Check for errors
grep -i error compile.log
```

**Expected:**
- Many compilation errors (expected at this stage)
- Missing Objective-C++ framework code
- Undefined Boxer functions

**Checkpoint:**
- [ ] BXCoalface files copied
- [ ] Build system recognizes Boxer option
- [ ] Attempts to compile .mm files
- [ ] Errors documented

---

## Phase 4: Implement Core Hooks (Week 2, Days 2-5)

### Objective

Add Boxer hooks to DOSBox source files.

### Step 4.1: Main Loop (src/dosbox.cpp)

```bash
cd ~/boxer-upgrade/dosbox-staging-new

# Edit src/dosbox.cpp
```

```cpp
// In DOSBOX_RunMachine() function, find the main loop

// OLD code:
void DOSBOX_RunMachine() {
    // ... setup ...

    while (true) {
        // emulation loop
    }
}

// NEW code:
void DOSBOX_RunMachine() {
    // ... setup ...

#ifdef BOXER_INTEGRATION
    while (boxer_runLoopShouldContinue()) {
#else
    while (true) {
#endif
        // emulation loop
    }
}

// In RunDOSBox() function:
void RunDOSBox() {
    // ... initialization ...

#ifdef BOXER_INTEGRATION
    boxer_runLoopWillStartWithContextInfo(nullptr);
#endif

    DOSBOX_RunMachine();

#ifdef BOXER_INTEGRATION
    boxer_runLoopDidFinishWithContextInfo(nullptr);
#endif

    // ... cleanup ...
}
```

**Test:**
```bash
meson compile -C build

# Should compile dosbox.cpp without errors now
```

### Step 4.2: Shell Integration (src/shell/*.cpp)

#### shell.cpp

```cpp
// In DOS_Shell::Run()
void DOS_Shell::Run() {
#ifdef BOXER_INTEGRATION
    boxer_shellWillStart();
#endif

    // ... shell loop ...

#ifdef BOXER_INTEGRATION
    boxer_shellDidFinish();
#endif
}
```

#### shell_cmds.cpp

```cpp
// In command execution
void DOS_Shell::ExecuteCommand(const char* command) {
#ifdef BOXER_INTEGRATION
    if (!boxer_shellShouldRunCommand(command)) {
        return;
    }
#endif

    // ... execute command ...
}
```

#### shell_misc.cpp

```cpp
// In program execution tracking
void DOS_Shell::ExecuteProgram(const char* name) {
#ifdef BOXER_INTEGRATION
    boxer_shellWillExecuteFileAtDOSPath(name);
#endif

    // ... execute ...
}
```

#### shell_batch.cpp

```cpp
// In batch file handling
void BatchFile::Start(const char* filename) {
#ifdef BOXER_INTEGRATION
    boxer_shellWillBeginBatchFile(filename);
#endif

    // ... start batch ...
}

void BatchFile::Close() {
#ifdef BOXER_INTEGRATION
    boxer_shellDidEndBatchFile(current_filename);
#endif

    // ... close ...
}
```

**Apply changes:**
```bash
cd ~/boxer-upgrade/dosbox-staging-new/src/shell

# Use your editor to add the #ifdef blocks
# OR use a script (be very careful with automated edits!)

# Test compilation
cd ../..
meson compile -C build
```

### Step 4.3: File System (src/dos/drive_local.cpp)

```cpp
// In file operations
bool localDrive::FileOpen(DOS_File** file, const char* name, uint32_t flags) {
#ifdef BOXER_INTEGRATION
    if (flags & OPEN_WRITE) {
        char full_path[DOS_PATHLENGTH];
        GetSystemPath(full_path, name);

        if (!boxer_shouldAllowWriteAccessToPath(full_path)) {
            DOS_SetError(DOSERR_ACCESS_DENIED);
            return false;
        }
    }
#endif

    // ... open file ...
}

bool localDrive::FileCreate(DOS_File** file, const char* name, uint16_t attributes) {
    // ... create file ...

#ifdef BOXER_INTEGRATION
    if (success) {
        char full_path[DOS_PATHLENGTH];
        GetSystemPath(full_path, name);
        boxer_didCreateLocalFile(full_path);
    }
#endif

    return success;
}

bool localDrive::FileUnlink(const char* name) {
#ifdef BOXER_INTEGRATION
    char full_path[DOS_PATHLENGTH];
    GetSystemPath(full_path, name);

    if (!boxer_shouldAllowWriteAccessToPath(full_path)) {
        DOS_SetError(DOSERR_ACCESS_DENIED);
        return false;
    }
#endif

    bool success = /* ... delete file ... */;

#ifdef BOXER_INTEGRATION
    if (success) {
        boxer_didRemoveLocalFile(full_path);
    }
#endif

    return success;
}
```

**Test:**
```bash
meson compile -C build
```

### Step 4.4: Rendering (src/gui/render/render.cpp)

**NOTE:** This is complex due to RenderBackend architecture.

**Recommended approach:** Create BoxerRenderBackend class.

Create `src/gui/render/boxer_backend.h`:

```cpp
#ifndef DOSBOX_BOXER_BACKEND_H
#define DOSBOX_BOXER_BACKEND_H

#ifdef BOXER_INTEGRATION

#include "gui/render/render_backend.h"
#include <cstdint>

class BoxerRenderBackend final : public RenderBackend {
public:
    BoxerRenderBackend() = default;
    ~BoxerRenderBackend() override = default;

    bool Initialize(SDL_Window* window, const uint16_t width,
                   const uint16_t height) override;
    void Teardown() override;

    SDL_Window* GetWindow() override;
    void StartFrame(uint8_t*& pixels_out, int& pitch_out) override;
    void EndFrame() override;
    void PrepareFrame() override;
    void PresentFrame() override;

    bool SetShader(const std::string& shader_name) override;
    // ... other required methods ...

private:
    SDL_Window* window_ = nullptr;
    uint8_t* framebuffer_ = nullptr;
    int pitch_ = 0;
};

#endif // BOXER_INTEGRATION
#endif // DOSBOX_BOXER_BACKEND_H
```

Create `src/gui/render/boxer_backend.cpp`:

```cpp
#ifdef BOXER_INTEGRATION

#include "boxer_backend.h"
#include "BXCoalface.h"

bool BoxerRenderBackend::Initialize(SDL_Window* window, const uint16_t width,
                                     const uint16_t height) {
    window_ = window;
    return true;
}

void BoxerRenderBackend::StartFrame(uint8_t*& pixels_out, int& pitch_out) {
    bool success = boxer_startFrame(&framebuffer_, &pitch_);
    if (success) {
        pixels_out = framebuffer_;
        pitch_out = pitch_;
    }
}

void BoxerRenderBackend::EndFrame() {
    boxer_finishFrame();
}

void BoxerRenderBackend::PrepareFrame() {
    // Boxer handles this
}

void BoxerRenderBackend::PresentFrame() {
    // Boxer handles presentation
}

SDL_Window* BoxerRenderBackend::GetWindow() {
    return window_;
}

bool BoxerRenderBackend::SetShader(const std::string& shader_name) {
    return boxer_setShader(shader_name.c_str());
}

// ... implement other required methods ...

#endif // BOXER_INTEGRATION
```

Update `src/gui/render/render.cpp`:

```cpp
#ifdef BOXER_INTEGRATION
#include "boxer_backend.h"
#endif

// In rendering initialization:
void GFX_InitializeRenderer() {
#ifdef BOXER_INTEGRATION
    render_backend = std::make_unique<BoxerRenderBackend>();
#else
    // Standard backends
    if (use_opengl) {
        render_backend = std::make_unique<OpenGLRenderer>();
    } else {
        render_backend = std::make_unique<SDLRenderer>();
    }
#endif

    render_backend->Initialize(/* ... */);
}
```

Update `src/gui/render/meson.build`:

```meson
render_sources = files(
    'render.cpp',
    'sdl_renderer.cpp',
    'opengl_renderer.cpp',
)

if get_option('boxer_integration')
    render_sources += files('boxer_backend.cpp')
endif

librender = static_library('render', render_sources, ...)
```

**Test:**
```bash
meson compile -C build
```

### Step 4.5: Input (src/hardware/input/*.cpp)

#### keyboard.cpp

```cpp
// Just ensure BXCoalface.h is included via dosbox.h
// Keyboard buffer functions should be accessible
```

#### bios_keyboard.cpp (src/ints/)

```cpp
// In key reading function
uint16_t BIOS_GetKeyCode() {
#ifdef BOXER_INTEGRATION
    if (boxer_numKeyCodesInPasteBuffer() > 0) {
        return boxer_getNextKeyCodeInPasteBuffer();
    }
#endif

    // ... normal input ...
}

// Lock key sync
void BIOS_SetCapsLock(bool state) {
    caps_lock = state;
#ifdef BOXER_INTEGRATION
    boxer_setCapsLockActive(state);
#endif
}

void BIOS_SetNumLock(bool state) {
    num_lock = state;
#ifdef BOXER_INTEGRATION
    boxer_setNumLockActive(state);
#endif
}
```

#### mouse.cpp

```cpp
void MOUSE_SetActive(bool active) {
    mouse_active = active;
#ifdef BOXER_INTEGRATION
    boxer_setMouseActive(active);
#endif
}
```

#### joystick.cpp

```cpp
void JOYSTICK_Enable(uint8_t which, bool enabled) {
    joystick_enabled[which] = enabled;
#ifdef BOXER_INTEGRATION
    boxer_setJoystickActive(which, enabled);
#endif
}
```

**Test:**
```bash
meson compile -C build
```

### Step 4.6: Audio/MIDI

#### src/audio/mixer.cpp

```cpp
float MIXER_GetMasterVolume() {
#ifdef BOXER_INTEGRATION
    return boxer_masterVolume();
#else
    return master_volume;
#endif
}
```

#### src/midi/midi.cpp

```cpp
void MidiHandler::PlayMsg(uint8_t* msg) {
#ifdef BOXER_INTEGRATION
    boxer_sendMIDIMessage(msg);
#else
    // ... normal MIDI handling ...
#endif
}

void MidiHandler::PlaySysex(uint8_t* sysex, size_t len) {
#ifdef BOXER_INTEGRATION
    boxer_sendMIDISysex(sysex, len);
#else
    // ... normal SysEx handling ...
#endif
}
```

**Test:**
```bash
meson compile -C build
```

**Checkpoint:**
- [ ] Core hooks implemented
- [ ] Compilation succeeds
- [ ] All #ifdef BOXER_INTEGRATION blocks added
- [ ] No new warnings

---

## Phase 5: Parallel Port Decision (Week 3, Day 1)

### Option A: Port Old Parport Code

```bash
cd ~/boxer-upgrade

# Copy old parport code
mkdir -p dosbox-staging-new/src/hardware/parport
cp -r dosbox-staging-old/src/hardware/parport/* \
      dosbox-staging-new/src/hardware/parport/

# Keep only needed files
cd dosbox-staging-new/src/hardware/parport
rm printer.cpp printer.h  # Full emulation not needed
rm filelpt.cpp filelpt.h  # If not used
rm directlpt_*.cpp directlpt_*.h  # Windows/Linux only

# Keep:
# - parport.cpp/h (infrastructure)
# - printer_redir.cpp/h (Boxer integration)
# - printer_if.h (interface)
```

Update `src/hardware/parport/printer_redir.cpp`:

```cpp
// Update includes for new structure
#include "parport.h"
#include "BXCoalface.h"

// Rest of file should work as-is
```

Update `src/hardware/meson.build`:

```meson
hardware_sources = files(
    # ... existing files ...
)

if get_option('boxer_integration')
    hardware_sources += files(
        'parport/parport.cpp',
        'parport/printer_redir.cpp',
    )
endif
```

**Test:**
```bash
meson compile -C build
```

### Option B: Stub Implementation

In `Boxer/BXCoalface.mm`:

```objc
Bitu boxer_PRINTER_readdata(Bitu port, Bitu iolen) {
    return 0xFF;  // No data available
}

void boxer_PRINTER_writedata(Bitu port, Bitu val, Bitu iolen) {
    // Silently discard
}

Bitu boxer_PRINTER_readstatus(Bitu port, Bitu iolen) {
    return 0x00;  // Not busy, no error
}

void boxer_PRINTER_writecontrol(Bitu port, Bitu val, Bitu iolen) {
    // Ignore
}

bool boxer_PRINTER_isInited(Bitu port) {
    return false;  // Printer not available
}

const char* boxer_PRINTER_getDeviceName(Bitu port) {
    return nullptr;
}
```

**No parport code needed in DOSBox**

---

## Phase 6: Testing (Weeks 3-4)

### Step 6.1: Link with Boxer

Update Boxer's Xcode project to use new DOSBox:

```bash
cd ~/boxer-upgrade/Boxer

# Update submodule or path
# This depends on how Boxer includes DOSBox
# If it's a submodule:
git submodule update --init
cd DOSBox-Staging
git checkout dosbox-boxer-upgrade-dosboxside
cd ..
git add DOSBox-Staging
git commit -m "Update DOSBox Staging submodule to new version"
```

### Step 6.2: Build Boxer

```bash
cd ~/boxer-upgrade/Boxer

# Open in Xcode
open Boxer.xcodeproj

# OR build from command line
xcodebuild -project Boxer.xcodeproj \
           -scheme Boxer \
           -configuration Release \
           build
```

**Expected issues:**
- Linker errors for unimplemented callbacks
- Runtime crashes on startup
- Missing framework links

**Fix systematically:**
1. Implement missing callbacks in BXCoalface.mm
2. Add required frameworks to Xcode project
3. Verify all include paths

### Step 6.3: Functional Testing

Create test checklist:

```bash
cd ~/boxer-upgrade

cat > test_checklist.md <<'EOF'
# Boxer Upgrade Test Checklist

## Basic Functionality
- [ ] Boxer launches
- [ ] DOSBox emulation starts
- [ ] Shell prompt appears
- [ ] Can type commands
- [ ] Can exit cleanly

## File System
- [ ] Can mount directories
- [ ] Can see files in mounted drives
- [ ] Can create files
- [ ] Can delete files
- [ ] Write protection works
- [ ] .DS_Store files hidden

## Shell
- [ ] Commands execute
- [ ] Batch files run
- [ ] AUTOEXEC.BAT processes
- [ ] Program execution tracked

## Rendering
- [ ] Graphics display correctly
- [ ] Text mode works
- [ ] Mode changes smooth
- [ ] Aspect ratio correct
- [ ] No visual glitches

## Input
- [ ] Keyboard input works
- [ ] Paste functionality works
- [ ] Caps Lock syncs
- [ ] Num Lock syncs
- [ ] Mouse captures/releases
- [ ] Joystick detected

## Audio
- [ ] Sound plays
- [ ] Volume control works
- [ ] No audio glitches

## MIDI
- [ ] MIDI output works
- [ ] Multiple devices supported

## Performance
- [ ] No noticeable slowdown
- [ ] CPU usage reasonable
- [ ] Memory usage stable

## Compatibility
- [ ] Test game 1:
- [ ] Test game 2:
- [ ] Test game 3:
- [ ] Windows 3.1:

## Stress Tests
- [ ] 1 hour continuous play
- [ ] Rapid pause/resume
- [ ] Multiple mount/unmount
- [ ] Heavy disk I/O

EOF
```

Work through checklist systematically.

### Step 6.4: Performance Profiling

```bash
# Build with profiling
cd ~/boxer-upgrade/dosbox-staging-new
meson configure build -Dtracy=enabled
meson compile -C build

# Run Tracy profiler
# Analyze hotspots
```

### Step 6.5: Memory Testing

```bash
# Build with sanitizers
meson configure build -Db_sanitize=address,undefined
meson compile -C build

# Run Boxer with this build
# Check for leaks or undefined behavior
```

---

## Phase 7: Documentation and Cleanup (Week 4)

### Step 7.1: Document Changes

Create changelog:

```bash
cat > BOXER_UPGRADE_CHANGELOG.md <<'EOF'
# Boxer DOSBox Staging Upgrade Changelog

## DOSBox Staging Version
- From: 0.78.0
- To: 0.83.0-alpha
- Commits: ~9000
- Time span: ~4-5 years

## Major Changes

### Architecture
- C++17 → C++20
- File reorganization (audio, rendering, input)
- New RenderBackend system
- CLAP audio plugin support

### Integration Changes
- Updated callback signatures (see INTEGRATION_MAPPING.md)
- New BoxerRenderBackend implementation
- Parallel port: [IMPLEMENTED/STUBBED/REMOVED]

### New Features Available
- Improved shaders (CRT, sharp scaling)
- SoundCanvas SC-55 emulation
- Better audio effects (reverb, chorus, crossfeed)
- Enhanced resampling (SpeexDSP)

### Breaking Changes
- [List any user-visible changes]

### Known Issues
- [List any remaining issues]

## Migration Notes
See MIGRATION_GUIDE.md for technical details.

EOF
```

### Step 7.2: Update Build Documentation

```bash
cat > BUILD_INSTRUCTIONS.md <<'EOF'
# Building Boxer with DOSBox Staging 0.83.0

## Prerequisites
- macOS 12.0+
- Xcode 14.0+
- Homebrew

## Install Dependencies
```bash
brew install meson cmake sdl2 sdl2_net opusfile fluidsynth libslirp speexdsp
```

## Clone and Build

```bash
git clone https://github.com/eduo/Boxer.git
cd Boxer
git submodule update --init

# Build DOSBox Staging
cd DOSBox-Staging
meson setup build -Dboxer_integration=true
meson compile -C build
cd ..

# Build Boxer
open Boxer.xcodeproj
# Build in Xcode
```

## Troubleshooting
[Add common issues and solutions]

EOF
```

### Step 7.3: Code Cleanup

```bash
# Remove backup files
find ~/boxer-upgrade -name "*.backup" -delete
find ~/boxer-upgrade -name "*.bak" -delete

# Format code (if using clang-format)
cd ~/boxer-upgrade/dosbox-staging-new
find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

### Step 7.4: Final Commit

```bash
cd ~/boxer-upgrade/dosbox-staging-new

git add -A
git commit -m "Complete Boxer integration for DOSBox Staging 0.83.0

- Updated all callback signatures
- Implemented BoxerRenderBackend
- Migrated file path changes
- [Parallel port: IMPLEMENTED/STUBBED/REMOVED]
- All integration tests passing

Closes #XXX"
```

---

## Rollback Procedure

If major issues discovered:

```bash
cd ~/boxer-upgrade/Boxer

# Revert submodule
cd DOSBox-Staging
git checkout <old-commit-hash>
cd ..

# OR switch back to old repository
rm -rf DOSBox-Staging
git submodule update --init
```

Keep old version in parallel until new version fully validated.

---

## Success Criteria

Project complete when:

- [ ] All 99 callbacks implemented
- [ ] All tests in checklist pass
- [ ] Performance within 5% of old version
- [ ] No memory leaks (verified with sanitizers)
- [ ] No known critical bugs
- [ ] Documentation complete
- [ ] User acceptance testing passed
- [ ] Code reviewed and approved

---

## Support and Resources

### Documentation
- BOXER_UPGRADE_STRATEGY.md - Overall strategy
- INTEGRATION_MAPPING.md - Technical details
- This file (MIGRATION_GUIDE.md) - Step-by-step instructions

### External Resources
- DOSBox Staging docs: https://dosbox-staging.github.io
- Meson docs: https://mesonbuild.com
- SDL2 docs: https://wiki.libsdl.org

### Getting Help
- Create issue in Boxer repository
- Reference this migration guide
- Include error logs and test results

---

**Last Updated:** 2025-11-14
**Document Version:** 1.0
