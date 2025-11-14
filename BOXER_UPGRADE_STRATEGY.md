# Boxer DOSBox Staging Upgrade Strategy

**Document Version:** 1.0
**Date:** 2025-11-14
**Author:** Claude (AI Analysis)

## Executive Summary

This document outlines the strategy for upgrading Boxer's DOSBox Staging integration from version 0.78.0 (current) to version 0.83.0-alpha (target). The upgrade spans approximately 9,000 commits and 4-5 years of development, including significant architectural changes, file reorganization, and the transition to C++20.

### Key Findings

**✅ Upgrade is VIABLE** but requires significant engineering effort (estimated 8-10 weeks)

**Critical Changes:**
- Language upgrade: C++17 → C++20
- Major file reorganization (audio, rendering, input subsystems separated)
- Rendering system refactored with new `RenderBackend` abstraction
- **Parallel port system removed** from DOSBox Staging
- Build system remains Meson (CMake experimental)
- 83 of 99 Boxer callbacks already prototyped in experimental branch

**Integration Complexity:**
- 22+ source files currently modified
- 50+ integration points throughout codebase
- Deep coupling via BXCoalface.h header injection
- C preprocessor macro-based function replacement

## Current State Analysis

### Version Information

| Aspect | Current (0.78.0) | Target (0.83.0-alpha) |
|--------|------------------|------------------------|
| **C++ Standard** | C++17 | C++20 |
| **Commit** | 0ae2d171a2ce | 4d8652275e3d |
| **Build System** | Meson | Meson (primary), CMake (experimental) |
| **Age** | ~4-5 years old | Current |
| **Repository** | eduo/dosbox-staging-boxer | eduo/dosbox-staging |

### Current Integration Approach

**Pattern:** Direct source code modification with callback injection

**Mechanism:**
1. `BXCoalface.h` included via `src/dosbox.h`
2. DOSBox functions replaced via `#define` macros
3. Boxer-specific code called at critical points
4. Objective-C++ bridge to BXEmulator class
5. Unified compilation (no separate libraries)

**Modified Files (22+):**
- Core: `dosbox.cpp`
- Rendering: `render.cpp`, `vga_other.cpp`
- Shell: `shell.cpp`, `shell_batch.cpp`, `shell_cmds.cpp`, `shell_misc.cpp`
- Filesystem: `drive_local.cpp`, `drive_cache.cpp`, `program_mount.cpp`
- Input: `keyboard.cpp`, `bios_keyboard.cpp`, `joystick.cpp`
- Audio: `mixer.cpp`, `midi.cpp`
- Printer: `printer_redir.cpp` (custom implementation)
- Messages: `messages.cpp`

### Integration Categories

| Category | Hooks | Complexity | Migration Difficulty |
|----------|-------|------------|---------------------|
| **Runloop Control** | 4 | Medium | Low |
| **Shell/Commands** | 12+ | High | Low |
| **File System** | 8+ | High | Low |
| **Rendering** | 10+ | Very High | **High** ⚠️ |
| **Input** | 10+ | Medium | Low |
| **Audio/MIDI** | 6 | Medium | Medium |
| **Printer** | 6 | Medium | **High** ⚠️ |
| **Localization** | 1 | Low | Low |

## Target Version Analysis

### Major Architectural Changes

#### 1. File Reorganization

**Audio Subsystem Separated:**
```
Old: src/hardware/mixer.cpp
New: src/audio/mixer.cpp
```

**Rendering Modularized:**
```
Old: src/gui/render.cpp
New: src/gui/render/render.cpp
     src/gui/render/sdl_renderer.cpp
     src/gui/render/opengl_renderer.cpp
```

**Input Centralized:**
```
Old: src/hardware/keyboard.cpp
     src/hardware/mouse.cpp
     src/hardware/joystick.cpp
New: src/hardware/input/keyboard.cpp
     src/hardware/input/mouse.cpp
     src/hardware/input/joystick.cpp
```

**Video Hardware Separated:**
```
Old: src/hardware/vga_*.cpp
New: src/hardware/video/vga_*.cpp
```

#### 2. New Abstraction Layers

**RenderBackend System** (`src/gui/render/render_backend.h`)
```cpp
class RenderBackend {
    virtual void StartFrame(uint8_t*& pixels_out, int& pitch_out) = 0;
    virtual void EndFrame() = 0;
    virtual void PrepareFrame() = 0;
    virtual void PresentFrame() = 0;
    virtual bool SetShader(const std::string& shader_name) = 0;
    // ...
};
```

**Implementations:**
- `SDLRenderer` - Software/texture rendering
- `OpenGLRenderer` - Hardware accelerated with shaders

**Opportunity:** Implement custom `BoxerRenderBackend` for cleaner integration

#### 3. CLAP Audio Plugin System

**Location:** `src/audio/clap/`

**Purpose:** Dynamic audio plugin loading (e.g., Nuked SC-55)

**Components:**
- Plugin interface
- Plugin manager
- Dynamic library loading
- MIDI event handling

**Relevance:** Could provide cleaner MIDI/audio integration path

#### 4. Enhanced Logging and Profiling

**Loguru Integration:** `src/libs/loguru/`
- Structured logging
- Better debugging capabilities
- Tracy profiler support

### Breaking Changes

#### ❌ Parallel Port Removed

**Impact:** HIGH - Boxer currently uses printer redirection

**Old Location:** `src/hardware/parport/*`

**Files That Existed:**
- `parport.cpp/h` - Port infrastructure
- `printer.cpp/h` - Full ESC/P emulation
- `printer_redir.cpp/h` - Boxer-specific redirection (KEY FILE)
- `filelpt.cpp/h` - File output
- `directlpt_*.cpp/h` - Hardware access

**Current Boxer Integration:**
```cpp
// printer_redir.cpp calls Boxer functions
Bitu CPrinterRedir::Read_PR() {
    return boxer_PRINTER_readdata(0,1);
}

bool CPrinterRedir::Putchar(Bit8u val) {
    Write_PR(val);
    return true;
}
```

**Options for Migration:**
1. **Port parport code forward** - Maintain as Boxer-specific code
2. **Serial port redirection** - Redirect printer to serial
3. **Drop printer support** - If usage is minimal
4. **LPT DAC only** - Keep audio features (`src/hardware/audio/lpt_dac.cpp` exists)

**Recommendation:** Survey user usage, then decide. If needed, maintain parport as Boxer-specific module.

#### ⚠️ Rendering API Changes

**Function Signature Changes:**
```cpp
// Old (0.78.0)
void GFX_StartUpdate()
void GFX_EndUpdate()

// New (0.83.0)
bool GFX_StartUpdate(uint8_t*& pixels, int& pitch)
void GFX_EndUpdate()
```

**Aspect Ratio Handling:**
```cpp
// Old
float aspect_ratio

// New
Fraction aspect_ratio  // Custom type with numerator/denominator
```

**Impact:** Moderate - Requires callback signature updates

#### ⚠️ Configuration System

**Reorganization:** `src/misc/setup.cpp` → `src/config/`

**Impact:** Low - Mostly internal changes

#### C++20 Language Requirements

**Compiler Requirements:**
- **Xcode:** Latest version with C++20 support
- **Apple Clang:** Xcode Command Line Tools (latest)
- **Features Used:** `<filesystem>`, concepts, ranges

**Impact:** May require developer environment updates

## Migration Strategy

### Phase 1: Foundation (Weeks 1-2)

**Objectives:**
- Update build environment to C++20
- Fix all include paths for reorganized files
- Establish compilation baseline

**Tasks:**

1. **Environment Setup**
   ```bash
   # Verify Xcode version
   xcode-select --version

   # Install dependencies
   brew install meson cmake ccache sdl2 sdl2_net opusfile \
                fluidsynth libslirp speexdsp pkg-config python3
   ```

2. **Clone Target Repository**
   ```bash
   git clone https://github.com/eduo/dosbox-staging.git dosbox-staging-new
   cd dosbox-staging-new
   git checkout dosbox-boxer-upgrade-dosboxside
   ```

3. **Update Include Paths**
   - Scan all Boxer integration code for old includes
   - Create mapping table (see Appendix A)
   - Update systematically

   Example changes:
   ```cpp
   // Old
   #include "mixer.h"

   // New
   #include "audio/mixer.h"
   ```

4. **Baseline Compilation**
   ```bash
   meson setup build
   meson compile -C build
   ```

   Expected result: DOSBox Staging compiles without Boxer integration

**Deliverables:**
- ✅ Clean DOSBox Staging build on macOS
- ✅ Updated include path mapping
- ✅ C++20 compilation confirmed

### Phase 2: BXCoalface Migration (Weeks 3-4)

**Objectives:**
- Port BXCoalface.h/mm to new structure
- Update all callback signatures
- Restore basic integration

**Tasks:**

1. **Copy BXCoalface Files**
   ```bash
   # From Boxer repo
   cp Boxer/BXCoalface.h dosbox-staging-new/src/
   cp Boxer/BXCoalface.mm dosbox-staging-new/src/
   cp Boxer/BXCoalfaceAudio.h dosbox-staging-new/src/
   cp Boxer/BXCoalfaceAudio.mm dosbox-staging-new/src/
   ```

2. **Update BXCoalface.h**
   - Fix all include paths to new locations
   - Update macro replacements for new APIs
   - Add new callback declarations

   Example:
   ```cpp
   // Update includes
   #include "audio/mixer.h"          // was "mixer.h"
   #include "hardware/input/keyboard.h"  // was "keyboard.h"
   #include "gui/render/render.h"    // was "render.h"
   ```

3. **Update Callback Signatures**
   ```cpp
   // Example: Update rendering callbacks
   bool boxer_startFrame(uint8_t*& pixels, int& pitch);  // Added parameters
   uint8_t boxer_prepareForFrameSize(int width, int height,
                                      Fraction aspect_ratio,  // Changed type
                                      uint8_t flags,
                                      VideoMode& mode,  // Added parameter
                                      GFX_Callback_t callback);
   ```

4. **Inject BXCoalface.h**
   - Modify `src/dosbox.h` to include BXCoalface.h
   - Ensure it's included after all standard headers

   ```cpp
   // src/dosbox.h
   // ... existing includes ...

   #ifdef BOXER_INTEGRATION
   #include "BXCoalface.h"
   #endif
   ```

5. **Test Compilation**
   ```bash
   meson configure build -DBOXER_INTEGRATION=true
   meson compile -C build
   ```

**Deliverables:**
- ✅ BXCoalface.h updated for new structure
- ✅ All callbacks declared with correct signatures
- ✅ Compilation succeeds with -DBOXER_INTEGRATION

### Phase 3: Core Integration (Weeks 4-5)

**Objectives:**
- Implement essential callback hooks
- Restore main loop control
- Enable basic Boxer functionality

**Priority 1: Main Loop Control**

**File:** `src/dosbox.cpp`

**Hooks:**
```cpp
// In main emulation loop
while (boxer_runLoopShouldContinue()) {
    // ... emulation code ...
}

// Around run loop
boxer_runLoopWillStartWithContextInfo(info);
// ... run loop ...
boxer_runLoopDidFinishWithContextInfo(info);
```

**Status:** Low complexity, essential for Boxer control

**Priority 2: Shell Integration**

**Files:**
- `src/shell/shell.cpp`
- `src/shell/shell_cmds.cpp`
- `src/shell/shell_misc.cpp`
- `src/shell/shell_batch.cpp`

**Key Hooks:**
```cpp
// Command interception
if (!boxer_shellShouldRunCommand(command)) {
    return;
}

// Execution tracking
boxer_shellWillExecuteFileAtDOSPath(path);

// Batch file tracking
boxer_shellWillBeginBatchFile(name);
boxer_shellDidEndBatchFile(name);
```

**Status:** Medium complexity, critical for program tracking

**Priority 3: Filesystem Integration**

**Files:**
- `src/dos/drive_local.cpp`
- `src/dos/drive_cache.cpp`
- `src/dos/programs/mount.cpp`

**Key Hooks:**
```cpp
// Access control
if (!boxer_shouldAllowWriteAccessToPath(path)) {
    DOS_SetError(DOSERR_ACCESS_DENIED);
    return false;
}

// Change notification
boxer_didCreateLocalFile(path);
boxer_didRemoveLocalFile(path);

// Mount tracking
boxer_driveDidMount(drive, path);
```

**Status:** Low complexity, important for security

**Priority 4: Input Handling**

**Files:**
- `src/hardware/input/keyboard.cpp` (NEW PATH)
- `src/ints/bios_keyboard.cpp`
- `src/hardware/input/mouse.cpp` (NEW PATH)
- `src/hardware/input/joystick.cpp` (NEW PATH)

**Key Hooks:**
```cpp
// Keyboard paste
if (boxer_numKeyCodesInPasteBuffer() > 0) {
    return boxer_getNextKeyCodeInPasteBuffer();
}

// Lock key sync
boxer_setCapsLockActive(state);
boxer_setNumLockActive(state);

// Mouse tracking
boxer_setMouseActive(active);
boxer_mouseMovedToPoint(x, y);
```

**Status:** Low complexity, update paths only

**Deliverables:**
- ✅ Main loop controllable from Boxer
- ✅ Shell commands interceptable
- ✅ File access controlled
- ✅ Input handling restored
- ✅ Basic Boxer functionality working

### Phase 4: Rendering System (Weeks 5-6)

**Objectives:**
- Implement custom RenderBackend
- Integrate with Boxer's rendering pipeline
- Handle aspect ratio and display modes

**Approach: Create BoxerRenderBackend**

**File:** Create `src/gui/render/boxer_backend.h` and `boxer_backend.cpp`

**Implementation:**
```cpp
// boxer_backend.h
#ifndef DOSBOX_BOXER_BACKEND_H
#define DOSBOX_BOXER_BACKEND_H

#include "gui/render/render_backend.h"

class BoxerRenderBackend final : public RenderBackend {
public:
    BoxerRenderBackend() = default;
    ~BoxerRenderBackend() override = default;

    // Required virtual methods
    SDL_Window* GetWindow() override;
    void StartFrame(uint8_t*& pixels_out, int& pitch_out) override;
    void EndFrame() override;
    void PrepareFrame() override;
    void PresentFrame() override;

    bool SetShader(const std::string& shader_name) override;
    bool Initialize(SDL_Window* window, const uint16_t width,
                   const uint16_t height) override;
    void Teardown() override;

    // ... other required methods ...

private:
    SDL_Window* sdl_window = nullptr;
    uint8_t* framebuffer = nullptr;
    int framebuffer_pitch = 0;
    int current_width = 0;
    int current_height = 0;
};

#endif
```

**Implementation Pattern:**
```cpp
// boxer_backend.cpp
#include "boxer_backend.h"
#include "BXCoalface.h"

void BoxerRenderBackend::StartFrame(uint8_t*& pixels_out, int& pitch_out) {
    // Call Boxer's callback
    bool success = boxer_startFrame(&framebuffer, &framebuffer_pitch);

    if (success) {
        pixels_out = framebuffer;
        pitch_out = framebuffer_pitch;
    }
}

void BoxerRenderBackend::EndFrame() {
    // Notify Boxer frame is complete
    boxer_finishFrame();
}

void BoxerRenderBackend::PresentFrame() {
    // Boxer handles presentation
    // No-op or minimal work here
}
```

**Integration:**
```cpp
// In rendering initialization code
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
```

**Display Mode Hooks:**

**File:** `src/hardware/video/vga_other.cpp` (NEW PATH)

```cpp
// CGA/Hercules display modes
uint8_t boxer_herculesTintMode();
void boxer_setHerculesTintMode(uint8_t mode);

float boxer_CGACompositeHueOffset();
void boxer_setCGACompositeHueOffset(float offset);
```

**Aspect Ratio Handling:**
```cpp
// Convert Fraction to float for Boxer
float aspect_float = static_cast<float>(aspect.numerator) /
                     static_cast<float>(aspect.denominator);
boxer_prepareForFrameSize(width, height, aspect_float, flags, mode);
```

**Deliverables:**
- ✅ BoxerRenderBackend implemented
- ✅ Frame data flowing to Boxer
- ✅ Display mode controls working
- ✅ Aspect ratio handling correct
- ✅ Shader selection functional (if used)

### Phase 5: Audio/MIDI (Week 6-7)

**Objectives:**
- Restore audio routing to Boxer
- Integrate MIDI handling
- Maintain volume control

**Audio Mixer Integration**

**File:** `src/audio/mixer.cpp` (NEW PATH)

**Hooks:**
```cpp
// Volume control
float mixer_volume = boxer_masterVolume();

// Apply to mixer
MIXER_SetMasterVolume(mixer_volume * 100.0f);
```

**Status:** Simple, just path updates

**MIDI Integration**

**File:** `src/midi/midi.cpp`

**Hooks:**
```cpp
// In MidiHandler::PlayMsg
boxer_sendMIDIMessage(msg);

// In MidiHandler::PlaySysex
boxer_sendMIDISysex(sysex, len);

// Device initialization
boxer_MIDIDeviceDidInitialize(device_name);
```

**New MIDI Devices to Consider:**
- `MidiDeviceSoundCanvas` - SC-55 emulation (new in 0.83.0)
- CoreMIDI improvements
- FluidSynth 2.5.0 features

**Deliverables:**
- ✅ Audio routing through Boxer
- ✅ MIDI messages forwarded
- ✅ Volume control functional
- ✅ MIDI device initialization tracked

### Phase 6: Parallel Port Decision (Week 7)

**Objectives:**
- Assess printer usage in Boxer
- Choose implementation strategy
- Implement chosen solution

**Decision Matrix:**

| Option | Effort | Risk | User Impact |
|--------|--------|------|-------------|
| **1. Port old parport code** | High | High | None |
| **2. Serial port redirect** | Medium | Medium | Different config |
| **3. Drop printer support** | Low | Low | Feature loss |
| **4. LPT DAC only** | Low | Low | No printing |

**Recommended Process:**

1. **User Research** (Days 1-2)
   - Survey Boxer users about printer usage
   - Check analytics/logs for printer feature usage
   - Review support tickets for printer issues

2. **Technical Assessment** (Days 3-4)
   - Evaluate old parport code quality
   - Check compatibility with new DOSBox
   - Estimate porting effort

3. **Implementation** (Days 5-7)

**Option 1: Port Parport Code (if needed)**

```bash
# Copy old parport directory
mkdir -p dosbox-staging-new/src/hardware/parport
cp -r dosbox-staging-old/src/hardware/parport/* \
      dosbox-staging-new/src/hardware/parport/

# Update for new DOSBox
# - Fix includes
# - Update build system
# - Test integration
```

**Files to port:**
- `parport.cpp/h` - Core infrastructure
- `printer_redir.cpp/h` - Boxer integration (CRITICAL)
- `printer_if.h` - Interface definition

**Files NOT to port (unless needed):**
- `printer.cpp/h` - Full emulation (Boxer doesn't use)
- `filelpt.cpp/h` - File output (if not used)
- `directlpt_*.cpp/h` - Hardware access (not applicable to macOS)

**Update Meson Build:**
```meson
# In src/hardware/meson.build
if get_option('enable_parport')
    hardware_sources += [
        'parport/parport.cpp',
        'parport/printer_redir.cpp',
    ]
endif
```

**Option 2: Drop Printer Support**

If usage is minimal:
```cpp
// Stub implementation in BXCoalface.mm
Bitu boxer_PRINTER_readdata(Bitu port, Bitu iolen) {
    return 0xFF;  // No data
}

bool boxer_PRINTER_writedata(Bitu port, Bitu val, Bitu iolen) {
    // Silently ignore
    return true;
}

// ... other stubs ...
```

**Deliverables:**
- ✅ Printer strategy decided
- ✅ Implementation complete (if needed)
- ✅ Documentation updated
- ✅ User communication prepared (if feature dropped)

### Phase 7: Localization & Messages (Week 7)

**Objectives:**
- Restore message localization
- Ensure Boxer strings used

**File:** `src/misc/messages.cpp`

**Hook:**
```cpp
// In MSG_Get
const char* MSG_Get(const char* key) {
    const char* localized = boxer_localizedStringForKey(key);
    if (localized) {
        return localized;
    }

    // Fall back to DOSBox default
    return default_messages[key];
}
```

**Status:** Simple, API unchanged

**Deliverables:**
- ✅ Localization working
- ✅ Boxer strings displayed

### Phase 8: Testing & Refinement (Weeks 8-10)

**Objectives:**
- Comprehensive testing of all integration points
- Performance optimization
- Bug fixes
- Documentation

**Testing Plan:**

**Week 8: Functional Testing**

1. **Core Functionality**
   - [ ] DOSBox starts and runs
   - [ ] Games launch successfully
   - [ ] Shell commands work
   - [ ] Keyboard input functions
   - [ ] Mouse control works
   - [ ] Joystick support operational

2. **Integration Points**
   - [ ] Boxer can pause/resume emulation
   - [ ] Shell command interception working
   - [ ] File access control enforced
   - [ ] Mount restrictions honored
   - [ ] File change notifications triggered

3. **Rendering**
   - [ ] Frame updates display correctly
   - [ ] Aspect ratio preserved
   - [ ] Display mode changes work
   - [ ] CGA/Hercules modes functional
   - [ ] No visual glitches

4. **Audio/MIDI**
   - [ ] Sound output correct
   - [ ] MIDI playback works
   - [ ] Volume control functional
   - [ ] Multiple audio devices supported

5. **Input**
   - [ ] Keyboard paste works
   - [ ] Lock keys sync with macOS
   - [ ] Mouse capture/release smooth
   - [ ] Joystick mapping correct

**Week 9: Edge Cases & Compatibility**

1. **Stress Testing**
   - [ ] Long-running games (24+ hours)
   - [ ] Rapid pause/resume cycles
   - [ ] Multiple mount/unmount operations
   - [ ] Heavy disk I/O games

2. **Compatibility Testing**
   - [ ] Test popular DOS games library
   - [ ] Windows 3.x/95 compatibility
   - [ ] CD-ROM games
   - [ ] Network-enabled games (if supported)

3. **macOS Integration**
   - [ ] Full screen mode
   - [ ] Window resizing
   - [ ] Multiple displays
   - [ ] Retina display support
   - [ ] Metal rendering (if applicable)

**Week 10: Performance & Polish**

1. **Performance Profiling**
   ```bash
   # Build with Tracy profiler
   meson configure build -Dtracy=enabled
   meson compile -C build

   # Profile hotspots
   # Optimize critical paths
   ```

2. **Memory Analysis**
   ```bash
   # Build with sanitizers
   meson configure build -Db_sanitize=address,undefined
   meson compile -C build

   # Test for leaks
   ```

3. **Bug Fixes**
   - Prioritize by severity
   - Focus on regressions from old version
   - Document known issues

4. **Documentation**
   - Update integration guide
   - Document new features available
   - Note breaking changes for users

**Deliverables:**
- ✅ All tests passing
- ✅ Performance acceptable (within 5% of old version)
- ✅ No memory leaks
- ✅ Critical bugs fixed
- ✅ Documentation complete

## Risk Assessment & Mitigation

### High Risk Items

#### 1. Rendering System Changes
**Risk:** RenderBackend integration fails or performs poorly

**Mitigation:**
- Create prototype early (Phase 4)
- Maintain fallback to old rendering code during transition
- Implement feature flag to switch between old/new

**Rollback Plan:**
- Keep old DOSBox version in parallel branch
- Can revert if rendering issues unsolvable

#### 2. Parallel Port Removal
**Risk:** Essential feature loss if users depend on printing

**Mitigation:**
- Early user research (Phase 6)
- Provide migration path if feature needed
- Maintain old parport code separately if necessary

**Rollback Plan:**
- Port old parport code forward
- Maintain as Boxer-specific module

#### 3. C++20 Compilation Issues
**Risk:** Build environment or compatibility problems

**Mitigation:**
- Test build environment early (Phase 1)
- Ensure latest Xcode installed
- Check all dependencies support C++20

**Rollback Plan:**
- If C++20 blocker found, stay on 0.78.0 or find intermediate version

#### 4. Performance Regression
**Risk:** New DOSBox version slower than old

**Mitigation:**
- Establish performance baseline before upgrade
- Profile throughout integration
- Optimize hotspots identified

**Rollback Plan:**
- Revert to old version if >10% performance loss

### Medium Risk Items

#### 1. MIDI System Changes
**Risk:** CoreMIDI integration behaves differently

**Mitigation:**
- Test early with real MIDI hardware
- Validate against known MIDI-heavy games

#### 2. Input Timing Changes
**Risk:** Keyboard/mouse feel different

**Mitigation:**
- Side-by-side testing with old version
- User acceptance testing
- Fine-tune timing parameters

#### 3. Build System Issues
**Risk:** Meson/CMake integration with Xcode problematic

**Mitigation:**
- Test both build systems
- Choose most reliable
- Document build process clearly

### Low Risk Items

#### 1. File Path Changes
**Risk:** Broken includes

**Mitigation:**
- Systematic find-and-replace
- Compilation will catch errors

#### 2. Shell Command API
**Risk:** Command handling changes

**Mitigation:**
- API is stable
- Minimal changes expected

#### 3. Localization
**Risk:** Message system incompatibility

**Mitigation:**
- Simple callback, minimal changes

## Alternative Approaches

### Approach A: Full Replacement (Recommended)

**Description:** Complete upgrade to 0.83.0-alpha as outlined above

**Pros:**
- Access to 4-5 years of improvements
- Bug fixes and security updates
- Modern C++20 codebase
- Better architecture for future

**Cons:**
- High effort (8-10 weeks)
- Risk of regressions
- Parallel port issue

**Recommendation:** Best long-term solution

### Approach B: Selective Backporting

**Description:** Stay on 0.78.0, backport specific features from 0.83.0

**Pros:**
- Lower risk
- Incremental progress
- Keep printer support

**Cons:**
- Miss most improvements
- Increasing technical debt
- Harder to maintain over time

**Recommendation:** Only if full upgrade blocked

### Approach C: Hybrid (Dual Version)

**Description:** Support both old and new DOSBox versions

**Pros:**
- No forced migration
- Users choose
- Fallback option

**Cons:**
- Double maintenance burden
- Confusing for users
- Delays inevitable migration

**Recommendation:** Not advised except as temporary measure

### Approach D: DOSBox Staging Plugin System

**Description:** Work with DOSBox Staging team to create official frontend API

**Pros:**
- Clean separation
- Upstreamable
- Easier future upgrades
- Benefit entire community

**Cons:**
- Very long timeline (months/years)
- Depends on DOSBox team
- May not meet all Boxer needs

**Recommendation:** Long-term goal, parallel to upgrade

## Success Criteria

### Phase Completion Criteria

Each phase complete when:
- [ ] All phase tasks completed
- [ ] Code compiles without warnings
- [ ] Automated tests pass (if applicable)
- [ ] Manual testing checklist complete
- [ ] Documentation updated
- [ ] Code reviewed

### Project Completion Criteria

Project successful when:
- [ ] All 99 Boxer callbacks functional
- [ ] All user-facing features working
- [ ] Performance within 5% of old version
- [ ] No critical bugs
- [ ] Documentation complete
- [ ] User acceptance testing passed
- [ ] Release candidate approved

### Quality Metrics

**Code Quality:**
- No memory leaks (verified with sanitizers)
- No undefined behavior (verified with UBSan)
- Clean compilation (no warnings with -Wall -Wextra)

**Performance:**
- Frame rate within 5% of old version
- CPU usage comparable
- Memory usage not significantly increased

**Stability:**
- No crashes in 24-hour stress test
- All regression tests passing
- No known critical bugs

## Timeline & Milestones

### Optimistic Timeline (8 weeks)

| Week | Phase | Milestone |
|------|-------|-----------|
| 1-2 | Foundation | ✅ Clean build of new DOSBox |
| 3-4 | BXCoalface & Core | ✅ Basic Boxer control working |
| 5-6 | Rendering & Audio | ✅ Output working correctly |
| 7 | Parallel Port & Messages | ✅ All features decided |
| 8-10 | Testing & Polish | ✅ Release ready |

### Realistic Timeline (10 weeks)

Same as above plus:
- Buffer time for unexpected issues
- Extended testing period
- User acceptance testing

### Conservative Timeline (12+ weeks)

If major blockers encountered:
- Rendering backend issues
- Parallel port full port required
- Performance optimization needed
- Significant bug fixing

## Resource Requirements

### Personnel

**Primary Developer:**
- Strong C++ knowledge (C++17/20)
- DOSBox internals familiarity
- macOS/Objective-C++ experience
- 8-10 weeks full-time

**Support:**
- QA/Testing: 2-3 weeks part-time
- User research: 1 week (for printer decision)
- Code review: Ongoing

### Infrastructure

**Development Environment:**
- macOS with latest Xcode
- 16GB+ RAM recommended
- Fast SSD for compilation

**Testing:**
- Multiple macOS versions (if possible)
- Various Mac hardware (Intel/Apple Silicon)
- Large DOS games library for testing

**Tools:**
- Meson, CMake
- Tracy profiler
- Sanitizers (ASan, UBSan)
- Version control (Git)

## Appendix A: Include Path Migration Table

| Old Include | New Include | Files Affected |
|-------------|-------------|----------------|
| `"mixer.h"` | `"audio/mixer.h"` | BXCoalface.h, mixer callbacks |
| `"render.h"` | `"gui/render/render.h"` | BXCoalface.h, rendering code |
| `"keyboard.h"` | `"hardware/input/keyboard.h"` | BXCoalface.h, input handling |
| `"mouse.h"` | `"hardware/input/mouse.h"` | Input callbacks |
| `"joystick.h"` | `"hardware/input/joystick.h"` | Input callbacks |
| `"vga.h"` | `"hardware/video/vga.h"` | Display mode code |
| `"setup.h"` | `"config/setup.h"` | Configuration code |

## Appendix B: Callback Migration Checklist

### Rendering (10 callbacks)
- [ ] `boxer_prepareForFrameSize()` - Update signature for Fraction
- [ ] `boxer_startFrame()` - Add pixel buffer parameters
- [ ] `boxer_finishFrame()` - No changes
- [ ] `boxer_idealOutputMode()` - No changes
- [ ] `boxer_getRGBPaletteEntry()` - No changes
- [ ] `boxer_setShader()` - New feature, implement if desired
- [ ] `boxer_herculesTintMode()` - Check VGA path change
- [ ] `boxer_setHerculesTintMode()` - Check VGA path change
- [ ] `boxer_CGACompositeHueOffset()` - Check VGA path change
- [ ] `boxer_setCGACompositeHueOffset()` - Check VGA path change

### Shell (14 callbacks)
- [ ] All shell callbacks - No expected changes
- [ ] Test command interception
- [ ] Test batch file tracking
- [ ] Test program execution tracking

### Drive/File (17 callbacks)
- [ ] All filesystem callbacks - No expected changes
- [ ] Test file access control
- [ ] Test mount restrictions
- [ ] Test change notifications

### Input (12 callbacks)
- [ ] Update keyboard includes
- [ ] Update mouse includes
- [ ] Update joystick includes
- [ ] Test paste functionality
- [ ] Test lock key syncing

### Audio/MIDI (6 callbacks)
- [ ] Update mixer includes
- [ ] Test volume control
- [ ] Test MIDI routing
- [ ] Test new SoundCanvas (optional)

### Printer (6 callbacks)
- [ ] Decide on printer strategy
- [ ] Implement chosen solution
- [ ] Test (if keeping printer)

### Other (10+ callbacks)
- [ ] Runloop control - Test
- [ ] Localization - Update
- [ ] Error handling - Test
- [ ] Logging - Update for Loguru

## Appendix C: Build Commands Reference

### Meson Build (Recommended)

```bash
# Initial setup
meson setup build -Dboxer_integration=true

# Compile
meson compile -C build

# Run tests (if available)
meson test -C build

# Install (optional)
meson install -C build

# Clean
rm -rf build
```

### CMake Build (Experimental)

```bash
# Configure
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" \
  -DBOXER_INTEGRATION=ON

# Build
cmake --build build

# Run specific target
cmake --build build --target dosbox
```

### Xcode Integration

```bash
# Generate Xcode project (if needed)
cmake -B build -G Xcode \
  -DBOXER_INTEGRATION=ON

# Open in Xcode
open build/dosbox-staging.xcodeproj
```

## Appendix D: Testing Checklist

See Phase 8 for detailed testing plan.

**Quick Smoke Test:**
- [ ] DOSBox launches
- [ ] Shell prompt appears
- [ ] Can mount a directory
- [ ] Can run a simple program
- [ ] Graphics display
- [ ] Sound plays
- [ ] Can exit cleanly

## Conclusion

The upgrade from DOSBox Staging 0.78.0 to 0.83.0-alpha is a significant but achievable undertaking. The primary challenges are the rendering system refactoring and the parallel port removal, but both have viable solutions. With careful planning, phased implementation, and thorough testing, Boxer can successfully modernize its DOSBox integration and benefit from years of upstream improvements.

**Next Steps:**
1. Review and approve this strategy
2. Secure resources (developer time, testing environment)
3. Make printer support decision (user research)
4. Begin Phase 1 (Foundation)
5. Regular progress reviews (weekly recommended)

**Key Success Factors:**
- Methodical phased approach
- Early prototyping of risky areas (rendering)
- Continuous testing throughout
- Clear rollback plans
- Good communication with users

---

**Document History:**
- v1.0 (2025-11-14): Initial analysis and strategy by Claude
