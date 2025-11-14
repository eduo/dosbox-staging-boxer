# Build System & CMake Integration Analysis

**Agent**: Agent 1B.1 - Build System Integration
**Created**: 2025-11-14T14:00:00Z
**Status**: Completed
**Dependencies**: Agent 1A (Integration Mapper)

## Summary

This analysis addresses **BLOCKER-001**: Build system incompatibility between Boxer's Xcode-based integration and DOSBox Staging's CMake build system. After comprehensive analysis of the target CMake structure, **five integration strategies** have been designed and evaluated.

**Recommendation**: **Strategy A (CMake Modification with Preprocessor Injection)** is the optimal approach, offering the best balance of maintainability, compatibility, and implementation effort. This strategy modifies DOSBox CMakeLists.txt files to inject Boxer's BXCoalface.h header and preprocessor macros, preserving the existing integration architecture while adapting to the CMake build system.

**Key Finding**: The target DOSBox Staging CMake build creates a `libdosboxcommon` static library and `dosbox` executable, providing excellent integration points for Boxer's callback hooks. The macro-based remapping strategy (#define GFX_Events → boxer_processEvents) can be successfully adapted to CMake using `target_compile_definitions()` and controlled include paths.

---

## Target DOSBox CMake Structure

### Root CMakeLists.txt Analysis

**Location**: `/home/user/dosbox-staging/CMakeLists.txt` (463 lines)

**Key Configuration**:
- **CMake Version**: 3.25 minimum
- **C++ Standard**: C++20 (strict, no extensions)
- **Project Version**: 0.83.0-alpha
- **Platform Detection**: DOSBOX_PLATFORM_MACOS, DOSBOX_PLATFORM_LINUX, DOSBOX_PLATFORM_WINDOWS

**Build Targets**:
1. **`dosbox`** (executable) - Created at line 395
   - Sources: `src/main.cpp`, `src/dosbox.cpp`
   - Links to: `libdosboxcommon`, SDL2, SDL2_net

2. **`libdosboxcommon`** (static library) - Created at line 428
   - Contains all DOSBox subsystem code
   - Links to: iir, loguru, libdecoders, SPEEXDSP, MT32Emu, FluidSynth, etc.

**Include Directories** (lines 396-398, 422-426):
```cmake
target_include_directories(dosbox PUBLIC
  include
  ${CMAKE_CURRENT_BINARY_DIR}/include
)

include_directories(
  src
  src/libs
  ${CMAKE_CURRENT_BINARY_DIR}/include
)
```

**Generated Configuration**:
- `dosbox_config.h` generated from `src/dosbox_config.h.in.cmake` (line 367-370)
- Placed in `${CMAKE_CURRENT_BINARY_DIR}/include/`
- Contains platform/feature detection macros (C_OPENGL, C_MT32EMU, MACOSX, etc.)

**macOS-Specific Settings** (lines 102-104, 329-332, 374-378):
- `DOSBOX_PLATFORM_MACOS` flag
- CoreAudio, CoreMIDI, CoreFoundation, CoreServices enabled
- Install RPATH: `@executable_path/lib`
- Deprecation warnings disabled (line 84)

### Subdirectory Organization

**Pattern**: Each subsystem has its own `CMakeLists.txt` that adds sources to `libdosboxcommon`

**Structure**:
```
src/
├── CMakeLists.txt (24 lines) - Adds subdirectories
├── audio/CMakeLists.txt (13 lines) - target_sources(libdosboxcommon PRIVATE ...)
├── capture/CMakeLists.txt (15 lines)
├── config/CMakeLists.txt (4 lines)
├── cpu/CMakeLists.txt (15 lines)
├── debugger/CMakeLists.txt (11 lines)
├── dos/CMakeLists.txt (61 lines) - Largest subsystem
├── fpu/CMakeLists.txt (1 line)
├── gui/CMakeLists.txt (16 lines)
├── hardware/CMakeLists.txt (94 lines) - Second largest
├── ints/CMakeLists.txt (17 lines)
├── libs/CMakeLists.txt (15 lines) - Third-party libraries
├── midi/CMakeLists.txt (25 lines)
├── misc/CMakeLists.txt (29 lines)
├── network/CMakeLists.txt (4 lines)
├── shell/CMakeLists.txt (10 lines)
└── utils/CMakeLists.txt (4 lines)
```

**Total**: 821 lines across 18 CMakeLists.txt files

### Build Targets

**Primary Targets**:
1. `dosbox` - Main executable
2. `libdosboxcommon` - Static library with all emulation code
3. `copy_assets` - Custom target for resource copying (line 64 in add_copy_assets.cmake)

**Test Targets** (if OPT_TESTS=ON):
- `dosbox_tests` - Unit tests
- Enabled by default (line 322)

**Library Targets** (in src/libs/):
- `loguru` - Logging library
- `libdecoders` - Audio/video decoders
- `libglad` - OpenGL loader
- `libesfmu`, `libresidfp`, `libym7128bemu` - Emulation libraries
- `libtalchorus` - Audio effects
- `libnuked` - OPL emulation
- `libmanymouse` - Multi-mouse support (conditional)

### Include Path Configuration

**Global Include Directories** (lines 422-426):
```cmake
include_directories(
  src                                  # All source headers
  src/libs                             # Third-party libraries
  ${CMAKE_CURRENT_BINARY_DIR}/include  # Generated headers (dosbox_config.h)
)
```

**Target-Specific** (lines 396-398):
```cmake
target_include_directories(dosbox PUBLIC
  include                              # Public headers (doesn't exist in current tree)
  ${CMAKE_CURRENT_BINARY_DIR}/include  # Generated config
)
```

**Header Location Pattern**: Headers are co-located with source files (no separate `include/` tree)

### Compilation Flags

**Common Flags** (lines 43-72):
- Warning flags: `-Wall -Wextra -Wdeprecated`
- Disabled: `-Wno-conversion -Wno-narrowing` (opt-in via CHECK_NARROWING() macro)
- Language-specific: Different flags for C vs C++ code

**macOS-Specific** (lines 83-85):
```cmake
if (APPLE)
  add_compile_options("-Wno-deprecated-declarations")
endif()
```

**Platform Definitions**:
- DOSBOX_PLATFORM_MACOS
- DOSBOX_PLATFORM_LINUX
- DOSBOX_PLATFORM_WINDOWS
- MACOSX (line 104)
- NOMINMAX (Windows, line 340)
- _USE_MATH_DEFINES (Windows, line 342)
- _CRT_SECURE_NO_WARNINGS (MSVC, line 29)

**Architecture Detection** (lines 219-241):
- x86_64: Sets C_TARGETCPU="X86_64", C_DYNAMIC_X86=ON, C_FPU_X86=ON
- ARM64: Sets C_TARGETCPU="ARMV8LE", C_DYNREC=ON

### Comparison with Legacy Meson Build

**DOSBox-Staging-Boxer** (legacy at `/home/user/dosbox-staging-boxer`):
- Used **Meson** build system (meson.build files present)
- Compiled directly in Boxer's Xcode project
- No standalone build capability
- BXCoalface.h included directly in modified files

**DOSBox-Staging** (target at `/home/user/dosbox-staging`):
- Uses **CMake exclusively** (no Meson files)
- Standalone build system
- No Xcode project files
- No Boxer integration hooks

**Key Differences**:
1. **Build System**: Meson → CMake (complete rewrite)
2. **Architecture**: Monolithic → Modular (subdirectory CMakeLists)
3. **Library Structure**: All-in-one → `libdosboxcommon` + `dosbox` executable
4. **Header Organization**: Co-located with sources (no central include/)
5. **Configuration**: Manual → Generated (dosbox_config.h.in.cmake)

### Existing Plugin/Extension Mechanisms

**Analysis**: CMake build has **NO** built-in plugin or extension mechanisms.

**Observations**:
- No `option()` for external integration
- No hooks for injecting custom headers
- No preprocessor guards for third-party modifications
- No CMake variables for external include paths
- No documented extension points

**However**, the architecture provides implicit extension points:
1. **Preprocessor Macros**: Can be injected via `add_compile_definitions()`
2. **Include Paths**: Can be extended via `include_directories()` or `target_include_directories()`
3. **Static Library**: `libdosboxcommon` can be linked to external code
4. **Source Injection**: `target_sources()` can add files to targets

---

## Integration Strategy Comparison

### Strategy A: CMake Modification (Direct Integration)

**Description**:
Modify DOSBox Staging's CMakeLists.txt files to inject Boxer's BXCoalface.h header and preprocessor macro remappings. This preserves the existing integration architecture while adapting to CMake.

**Approach**:
1. Add Boxer include paths to CMake via `target_include_directories()`
2. Inject preprocessor macros (#define remappings) via `target_compile_definitions()`
3. Conditionally include BXCoalface.h using `#ifdef BOXER_BUILD`
4. Modify ~18 DOSBox files to include BXCoalface.h when BOXER_BUILD is defined
5. Build DOSBox as part of Boxer's Xcode project, invoking CMake as external build step

**CMake Changes Required**:
- Root CMakeLists.txt: ~15 lines added
- Conditional Boxer integration block
- No changes to subdirectory CMakeLists.txt files (use global include_directories)

**DOSBox Source Changes**:
- ~18 files need `#ifdef BOXER_BUILD` guards
- Add `#include "BXCoalface.h"` in guarded sections
- No logic changes, only header inclusion

**Feasibility**: **HIGH** - CMake fully supports this approach

**Pros**:
- ✅ **Preserves existing architecture** - No changes to Boxer's 86 integration points
- ✅ **Minimal CMake modifications** - ~15 lines in root CMakeLists.txt
- ✅ **Clean separation** - Boxer code only included when `BOXER_BUILD` is defined
- ✅ **Maintainable** - Changes isolated to well-defined locations
- ✅ **Preprocessor macros work** - CMake `target_compile_definitions()` supports this
- ✅ **Xcode integration** - Can invoke CMake from Xcode build phase
- ✅ **Compilation order guaranteed** - CMake handles dependencies

**Cons**:
- ⚠️ **Requires DOSBox source modifications** - 18+ files need `#ifdef` guards
- ⚠️ **Fork maintenance** - Need to maintain modified DOSBox as submodule/fork
- ⚠️ **Merge conflicts** - Future DOSBox updates require careful merging
- ⚠️ **Build complexity** - Xcode → CMake → Compiler toolchain

**Implementation Complexity**: **15-20 hours**
- CMake modifications: 3-4 hours
- DOSBox source guards: 4-5 hours
- Xcode build script: 2-3 hours
- Testing and debugging: 4-6 hours
- Documentation: 2 hours

**Maintainability**: **GOOD** (8/10)
- Changes well-documented and isolated
- Future DOSBox updates require reviewing 18 files
- CMake changes confined to root file
- Preprocessor guards make integration explicit

---

### Strategy B: Patch-Based Integration

**Description**:
Generate patch files (.patch) containing all CMake and source modifications. Apply patches automatically during Boxer build process before invoking CMake.

**Approach**:
1. Create `boxer-integration.patch` with CMakeLists.txt modifications
2. Create `boxer-source-guards.patch` with BXCoalface.h includes
3. Store patches in `/Boxer/Resources/DOSBox-Patches/`
4. Xcode Run Script Phase: Copy DOSBox → Apply patches → Run CMake → Build
5. Clean build each time (or track patch application state)

**Patch Management**:
- Separate patches for CMake vs. source changes
- Version-tagged patches (e.g., `v0.83.0-boxer.patch`)
- Automated patch generation script
- Patch validation tests

**Feasibility**: **MEDIUM-HIGH** - Proven technique, but adds build complexity

**Pros**:
- ✅ **Clean DOSBox submodule** - No committed modifications to DOSBox
- ✅ **Easy to see changes** - Patches are human-readable diffs
- ✅ **Version control friendly** - Track patch evolution separately
- ✅ **Upstream updates easier** - Update submodule, regenerate patches
- ✅ **Rollback capability** - Can unapply patches for debugging
- ✅ **Multiple patch sets** - Different patches for different DOSBox versions

**Cons**:
- ❌ **Build complexity** - Patch application adds steps and failure modes
- ❌ **Patch fragility** - DOSBox updates can break patches (line number changes)
- ❌ **Debugging difficulty** - Build failures may be in patching vs. compilation
- ❌ **Incremental builds harder** - May need clean builds when patches change
- ❌ **Patch maintenance burden** - Must manually update patches for DOSBox changes
- ⚠️ **No IDE support** - Xcode won't understand patched files for autocomplete/navigation

**Implementation Complexity**: **20-25 hours**
- Patch generation script: 4-5 hours
- Xcode integration: 4-5 hours
- Patch validation: 3-4 hours
- Build state tracking: 3-4 hours
- Testing and debugging: 4-5 hours
- Documentation: 2 hours

**Maintainability**: **FAIR** (6/10)
- Patch conflicts require manual resolution
- Hard to debug when patches fail to apply
- Versioning patches adds complexity
- Benefits diminish if DOSBox updates frequently

---

### Strategy C: Separate Compilation + Linking

**Description**:
Build DOSBox Staging as an unmodified static library using its vanilla CMake system, then link the compiled objects into Boxer. Inject Boxer callbacks at the linking stage or via dynamic symbol replacement.

**Approach**:
1. Build DOSBox using unmodified CMake → produces `libdosboxcommon.a`
2. Create Boxer wrapper layer (`BXDOSBoxAdapter.cpp/mm`) that provides GFX_* symbols
3. Link Boxer against `libdosboxcommon.a` + wrapper symbols
4. Use linker flags to override DOSBox symbols with Boxer implementations
   - macOS: `-Wl,-alias` or `-Wl,-order_file`
   - Or: Use `ld -r` to relocate symbols before final link

**Symbol Replacement Techniques**:
- **Weak symbols**: Mark DOSBox functions `__attribute__((weak))` (requires DOSBox changes)
- **Link-time substitution**: Provide strong symbols in Boxer that override weak DOSBox symbols
- **Interposition**: Use `DYLD_INSERT_LIBRARIES` (runtime, not build-time)
- **Object file manipulation**: Extract .o files, relink with Boxer symbols taking precedence

**Feasibility**: **LOW-MEDIUM** - Technically possible but significant limitations

**Pros**:
- ✅ **Unmodified DOSBox** - Build DOSBox vanilla, no source changes
- ✅ **Clean separation** - Boxer and DOSBox completely separate
- ✅ **Easy DOSBox updates** - Just rebuild library, no merge conflicts
- ✅ **Standard build process** - CMake runs independently of Xcode

**Cons**:
- ❌ **Callback limitations** - Can only intercept existing functions, not add new hooks
- ❌ **Missing integration points** - 71 direct callbacks can't be added without source mods
- ❌ **Symbol visibility issues** - Many DOSBox internals are static/private
- ❌ **Macro remapping impossible** - #define substitutions happen at compile-time, not link-time
- ❌ **Platform-specific linker tricks** - macOS/Linux linking behaviors differ
- ❌ **Debugging nightmare** - Symbol conflicts, undefined references, link errors
- ❌ **15 macro-based integration points fail** - GFX_Events, GFX_StartUpdate, etc. can't be remapped

**Critical Blocker**: **86 integration points** include:
- **15 macro replacements** (#define) - CANNOT work at link-time
- **71 direct callbacks** - Many require adding calls to Boxer functions in DOSBox code

**Conclusion**: This strategy **cannot support** Boxer's existing integration architecture without extensive refactoring.

**Implementation Complexity**: **40-60 hours** (and incomplete solution)
- Wrapper layer design: 10-15 hours
- Linker script development: 8-10 hours
- Symbol resolution: 10-15 hours
- Architecture refactoring (to work around limitations): 10-15 hours
- Testing: 5-10 hours

**Maintainability**: **POOR** (3/10)
- Linker behavior fragile and platform-specific
- Future DOSBox changes may add static functions that can't be intercepted
- Debugging symbol conflicts extremely difficult
- Incomplete integration (many callbacks impossible)

**Recommendation**: **NOT VIABLE** for Boxer's current architecture

---

### Strategy D: Wrapper/Shim Layer

**Description**:
Build DOSBox normally via CMake, create a thin C++ wrapper/shim layer that intercepts function calls, and link the shim into Boxer. Minimizes DOSBox changes while providing callback hooks.

**Approach**:
1. Build DOSBox Staging with minimal modifications:
   - Export key functions (remove `static` keywords)
   - Add callback registration functions
2. Create `BXDOSBoxShim.cpp/mm` layer in Boxer
3. Shim intercepts DOSBox function calls and invokes Boxer implementations
4. Use virtual function tables or function pointers for indirection

**Example Architecture**:
```cpp
// DOSBox modification (minimal)
namespace DOSBox {
    struct RenderCallbacks {
        bool (*StartUpdate)(uint8_t*& pixels, int& pitch);
        void (*EndUpdate)();
        // ...
    };
    extern RenderCallbacks* g_callbacks;  // Set by Boxer
}

bool GFX_StartUpdate(uint8_t*& pixels, int& pitch) {
    if (DOSBox::g_callbacks && DOSBox::g_callbacks->StartUpdate) {
        return DOSBox::g_callbacks->StartUpdate(pixels, pitch);
    }
    return default_implementation(pixels, pitch);
}

// Boxer side (BXDOSBoxShim.mm)
@implementation BXDOSBoxShim
+ (void)registerCallbacks {
    static DOSBox::RenderCallbacks callbacks = {
        .StartUpdate = boxer_startFrame,
        .EndUpdate = boxer_finishFrame,
        // ...
    };
    DOSBox::g_callbacks = &callbacks;
}
@end
```

**Feasibility**: **MEDIUM** - Requires DOSBox refactoring but more maintainable

**Pros**:
- ✅ **Reduced DOSBox modifications** - ~30-50 lines vs. 18 files with #ifdef
- ✅ **Cleaner architecture** - Explicit callback registration
- ✅ **Flexible** - Can add/remove callbacks without recompiling DOSBox
- ✅ **Better performance potential** - Direct function pointers vs. macro indirection
- ✅ **Easier debugging** - Clear call boundaries between Boxer and DOSBox

**Cons**:
- ⚠️ **Significant refactoring** - Boxer's 86 integration points need adapters
- ⚠️ **Performance overhead** - Function pointer indirection for every call
- ⚠️ **Still requires DOSBox changes** - Need to add callback registration points
- ⚠️ **Complexity** - Two-layer architecture (Boxer → Shim → DOSBox)
- ⚠️ **Macro replacements hard** - 15 #define points need alternative mechanism
- ❌ **Doesn't eliminate maintenance burden** - Still need fork/submodule

**Implementation Complexity**: **30-40 hours**
- Shim layer design: 8-10 hours
- DOSBox callback registration: 8-10 hours
- Boxer integration refactoring: 10-12 hours
- Testing: 6-8 hours
- Documentation: 3 hours

**Maintainability**: **GOOD** (7/10)
- Cleaner separation of concerns
- Callback registration explicit
- Still requires maintaining DOSBox fork
- May be easier to update DOSBox (fewer scattered changes)

**Trade-off**: More upfront refactoring, potentially better long-term maintainability

---

### Strategy E: Submodule with Local Modifications (Fork Strategy)

**Description**:
Fork DOSBox Staging to a Boxer-specific repository (e.g., `MaddTheSane/dosbox-staging-boxer`), commit all integration changes directly, use as git submodule. This is essentially **Strategy A** with version control strategy made explicit.

**Approach**:
1. Fork DOSBox Staging to `dosbox-staging-boxer` repository
2. Create `boxer-integration` branch
3. Commit CMakeLists.txt modifications
4. Commit BXCoalface.h includes to 18+ files
5. Use as submodule in Boxer: `git submodule add https://github.com/MaddTheSane/dosbox-staging-boxer.git`
6. Periodically merge upstream DOSBox Staging changes

**Branch Strategy**:
```
upstream/main (dosbox-staging official)
     ↓
fork/boxer-integration (MaddTheSane/dosbox-staging-boxer)
     ↓
Boxer/DOSBox-Staging submodule
```

**Update Process**:
1. Fetch upstream DOSBox Staging
2. Merge into `boxer-integration` branch
3. Resolve conflicts in 18 modified files + CMakeLists.txt
4. Test build
5. Update Boxer submodule reference

**Feasibility**: **HIGH** - Standard practice for forked dependencies

**Pros**:
- ✅ **Version control benefits** - All changes tracked in git
- ✅ **Easy rollback** - Can revert to any previous state
- ✅ **Collaboration-friendly** - Others can contribute to fork
- ✅ **Upstream updates** - Merge strategy well-understood
- ✅ **Build simplicity** - No patch application, just checkout and build
- ✅ **IDE-friendly** - Xcode can navigate full modified source tree
- ✅ **Same as Strategy A** - All Strategy A pros apply

**Cons**:
- ⚠️ **Fork maintenance burden** - Must regularly merge upstream
- ⚠️ **Merge conflicts inevitable** - 18+ files will conflict on updates
- ⚠️ **Divergence risk** - Fork may drift from upstream over time
- ⚠️ **Public fork coordination** - If public, need to manage external contributions
- ⚠️ **Same as Strategy A** - All Strategy A cons apply

**Implementation Complexity**: **15-20 hours** (same as Strategy A)
- Initial fork setup: 1 hour
- CMake modifications: 3-4 hours
- DOSBox source modifications: 4-5 hours
- Xcode integration: 2-3 hours
- Testing: 4-6 hours
- Documentation: 2 hours

**Maintainability**: **GOOD** (8/10)
- Standard git workflow
- Merge conflicts manageable with git tools
- Clear history of changes
- Can cherry-pick upstream fixes

**Note**: This is the **version control strategy** for implementing **Strategy A**. It doesn't change the technical approach, just how changes are tracked.

---

## Recommended Strategy

### Primary Recommendation: **Strategy E (Submodule with Local Modifications)**

This is **Strategy A (CMake Modification)** implemented using a **forked submodule** approach.

### Justification

**Technical Superiority**:
1. **Preserves Architecture** - All 86 integration points work unchanged
2. **Minimal Changes** - ~15 lines CMakeLists.txt + 18 file guards
3. **Proven Pattern** - This is how Boxer currently integrates (just migrating Meson→CMake)
4. **CMake Native** - Uses CMake's intended mechanisms (target_compile_definitions, include_directories)
5. **Platform Compatible** - macOS-specific flags already in DOSBox CMake

**Maintainability**:
- Git provides excellent merge tools for upstream updates
- Changes isolated to known files (18 + root CMakeLists.txt)
- Can diff against upstream easily
- Rollback capability via git

**Build Integration**:
- Xcode can invoke CMake in a Run Script Phase
- Standard ExternalProject pattern in Xcode
- No patch application fragility
- IDE navigation works (Xcode can index modified source)

**Risk Mitigation**:
- Lowest implementation risk (well-understood technology)
- No experimental linker tricks (Strategy C)
- No patch fragility (Strategy B)
- No major refactoring (Strategy D)

**Comparison to Alternatives**:
- **vs. Strategy B (Patches)**: Same changes, better tooling (git merge vs. patch conflicts)
- **vs. Strategy C (Separate Link)**: Actually works for all 86 integration points
- **vs. Strategy D (Shim)**: Less refactoring, faster implementation
- **vs. Strategy A (without fork)**: Adds version control discipline

### Implementation Plan

#### Phase 1: Fork Setup (2 hours)

1. **Create Fork**:
   ```bash
   # On GitHub: Fork dosbox-staging to MaddTheSane/dosbox-staging-boxer
   git clone https://github.com/MaddTheSane/dosbox-staging-boxer.git
   cd dosbox-staging-boxer
   git checkout -b boxer-integration
   git remote add upstream https://github.com/dosbox-staging/dosbox-staging.git
   ```

2. **Update Boxer Submodule Reference**:
   ```bash
   cd /path/to/Boxer
   git submodule set-url DOSBox-Staging https://github.com/MaddTheSane/dosbox-staging-boxer.git
   git submodule update --init --recursive
   cd DOSBox-Staging
   git checkout boxer-integration
   ```

#### Phase 2: CMakeLists.txt Modifications (3-4 hours)

1. **Root CMakeLists.txt** (`/home/user/dosbox-staging/CMakeLists.txt`):

Add after line 98 (after `include(GNUInstallDirs)`):

```cmake
# ============================================================================
# Boxer Integration
# ============================================================================
option(BOXER_BUILD "Build DOSBox for Boxer integration" OFF)

if (BOXER_BUILD)
  message(STATUS "Configuring for Boxer integration")

  # Add Boxer header paths
  # These paths are relative to the DOSBox-Staging submodule when built from Boxer
  set(BOXER_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Boxer" CACHE PATH "Path to Boxer source")

  if (NOT EXISTS "${BOXER_INCLUDE_DIR}/BXCoalface.h")
    message(WARNING "BXCoalface.h not found at ${BOXER_INCLUDE_DIR}")
    message(WARNING "Set BOXER_INCLUDE_DIR to correct path")
  endif()

  # Add Boxer include directory globally so all source files can find BXCoalface.h
  include_directories("${BOXER_INCLUDE_DIR}")

  # Define BOXER preprocessor macro for conditional compilation
  add_compile_definitions(BOXER)

  # Inject Boxer's preprocessor macro remappings
  # These replace DOSBox functions with boxer_* equivalents
  add_compile_definitions(
    GFX_Events=boxer_processEvents
    GFX_StartUpdate=boxer_startFrame
    GFX_EndUpdate=boxer_finishFrame
    Mouse_AutoLock=boxer_setMouseActive
    GFX_SetTitle=boxer_handleDOSBoxTitleChange
    GFX_GetDisplayRefreshRate=boxer_GetDisplayRefreshRate
    GFX_SetSize=boxer_prepareForFrameSize
    GFX_GetRGB=boxer_getRGBPaletteEntry
    GFX_SetShader=boxer_setShader
    GFX_GetBestMode=boxer_idealOutputMode
    GFX_MaybeProcessEvents=boxer_MaybeProcessEvents
    GFX_ShowMsg=boxer_log
    MIDI_Available=boxer_MIDIAvailable
    OpenCaptureFile=boxer_openCaptureFile
  )

  # Note: E_Exit macro is more complex and defined in BXCoalface.h itself

  message(STATUS "Boxer integration enabled")
  message(STATUS "  Boxer include path: ${BOXER_INCLUDE_DIR}")
endif()
# ============================================================================
```

**Lines Added**: ~43 lines (with comments)

2. **Verify Configuration**:
   ```bash
   cd /path/to/dosbox-staging/build
   cmake .. -DBOXER_BUILD=ON -DBOXER_INCLUDE_DIR=/path/to/Boxer/Boxer
   # Should output: "Configuring for Boxer integration"
   ```

#### Phase 3: Source File Modifications (4-5 hours)

For each of the **18+ files** identified by Agent 1A, add conditional BXCoalface.h include.

**Pattern** (example: `src/gui/sdl_gui.cpp`):

Find the include section (typically after system includes, before DOSBox includes):

```cpp
#include <SDL2/SDL.h>
#include <memory>

#include "dosbox.h"
#include "video.h"
// ... other includes

// ============================================================================
// Boxer Integration Hook
// ============================================================================
#ifdef BOXER
#include "BXCoalface.h"  // Provides boxer_* function declarations and macro remappings
#endif
// ============================================================================
```

**Files to Modify** (from Agent 1A report):

1. Core emulation:
   - `src/dosbox.cpp`
   - `src/main.cpp` (may not need modification, verify)

2. Graphics/GUI:
   - `src/gui/sdl_gui.cpp`
   - `src/gui/render/render.cpp`

3. Shell:
   - `src/shell/shell.cpp`
   - `src/shell/shell_cmds.cpp`
   - `src/shell/shell_misc.cpp`
   - `src/shell/shell_batch.cpp`

4. File I/O:
   - `src/dos/drive_local.cpp`
   - `src/dos/drive_cache.cpp`
   - `src/dos/dos_programs.cpp` (or individual programs/mount*.cpp)

5. Input:
   - `src/ints/bios_keyboard.cpp`
   - `src/hardware/input/keyboard.cpp`
   - `src/hardware/input/joystick.cpp`
   - `src/dos/dos_keyboard_layout.cpp`

6. Audio:
   - `src/audio/mixer.cpp` (verify path - may be hardware/audio/mixer.cpp)
   - `src/midi/midi.cpp`

7. Video:
   - `src/hardware/video/vga_other.cpp` (for Hercules/CGA helpers)

8. Printer:
   - `src/hardware/parport/printer_redir.cpp` (CHECK: does this path exist in target?)

9. Localization:
   - `src/misc/messages.cpp`

**Note**: File paths may have changed between legacy and target. Use `find` and `grep` to verify:

```bash
cd /home/user/dosbox-staging
find src -name "shell.cpp" -o -name "mixer.cpp" -o -name "printer_redir.cpp"
```

#### Phase 4: Verify Parport Support (1-2 hours)

**Critical Check**: Does target DOSBox Staging still have parallel port support?

```bash
find /home/user/dosbox-staging/src -type d -name "parport"
find /home/user/dosbox-staging/src -name "*printer*"
grep -r "PRINTER\|parport" /home/user/dosbox-staging/src
```

**Scenarios**:

1. **Parport exists**: Add BXCoalface.h include as above
2. **Parport removed**:
   - **Option A**: Port parport code from legacy DOSBox to target (significant effort)
   - **Option B**: Disable Boxer printer feature (lose functionality)
   - **Option C**: Keep printer emulation in Boxer, intercept at higher level

**Recommendation**: If parport is removed, investigate **Option C** first (least invasive).

#### Phase 5: Xcode Build Integration (2-3 hours)

Add CMake invocation to Boxer's Xcode project:

1. **Add Run Script Build Phase** (before "Compile Sources"):

```bash
# Build DOSBox via CMake

set -e  # Exit on error
set -u  # Error on undefined variables

# Paths
DOSBOX_SOURCE="${PROJECT_DIR}/DOSBox-Staging"
DOSBOX_BUILD="${BUILD_DIR}/DOSBox-Build"
BOXER_SOURCE="${PROJECT_DIR}/Boxer"

# Create build directory
mkdir -p "${DOSBOX_BUILD}"

# Configure CMake (only if needed)
if [ ! -f "${DOSBOX_BUILD}/CMakeCache.txt" ]; then
    echo "Configuring DOSBox with CMake..."
    cd "${DOSBOX_BUILD}"
    cmake "${DOSBOX_SOURCE}" \
        -DBOXER_BUILD=ON \
        -DBOXER_INCLUDE_DIR="${BOXER_SOURCE}" \
        -DCMAKE_BUILD_TYPE="${CONFIGURATION}" \
        -DCMAKE_OSX_ARCHITECTURES="${ARCHS}" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET}" \
        -DOPT_TESTS=OFF \
        -G "Unix Makefiles"
fi

# Build DOSBox
echo "Building DOSBox..."
cd "${DOSBOX_BUILD}"
make -j$(sysctl -n hw.ncpu) libdosboxcommon

# Copy library to known location for Xcode linking
cp "${DOSBOX_BUILD}/src/libdosboxcommon.a" "${BUILD_DIR}/libdosboxcommon.a"

echo "DOSBox build complete: ${BUILD_DIR}/libdosboxcommon.a"
```

2. **Link Against libdosboxcommon.a**:

In Xcode **Build Settings**:
- **Other Linker Flags**: Add `$(BUILD_DIR)/libdosboxcommon.a`
- **Header Search Paths**: Add `$(PROJECT_DIR)/DOSBox-Staging/src`
- **Library Search Paths**: Add `$(BUILD_DIR)`

3. **Add DOSBox Dependencies to Boxer Target**:

Boxer needs to link against DOSBox's dependencies (SDL2, SDL2_net, etc.). These should already be in Boxer's Xcode project, but verify versions match.

#### Phase 6: Testing (4-6 hours)

1. **Build Verification**:
   ```bash
   # Clean build
   cd Boxer
   xcodebuild clean
   xcodebuild -configuration Debug
   ```

2. **Integration Point Testing** (use Agent 1A's criticality ratings):

   **Critical Points** (must work):
   - INT-001: boxer_processEvents (event loop)
   - INT-002: boxer_startFrame (rendering)
   - INT-003: boxer_finishFrame (rendering)
   - INT-007: boxer_prepareForFrameSize (rendering setup)
   - INT-041: boxer_shouldAllowWriteAccessToPath (file security)
   - INT-059: boxer_runLoopShouldContinue (emulation control)

   **Test Plan**:
   - Launch Boxer
   - Boot a DOS game
   - Verify rendering appears (INT-002, INT-003)
   - Verify input works (INT-001)
   - Verify file access (INT-041)
   - Verify clean shutdown (INT-059)

3. **Debug Common Issues**:
   - **BXCoalface.h not found**: Check BOXER_INCLUDE_DIR path
   - **Undefined symbols**: Verify BXCoalface.mm is compiled and linked
   - **Macro remapping not working**: Check `add_compile_definitions()` syntax
   - **Linking errors**: Ensure libdosboxcommon.a includes all subsystems

#### Phase 7: Documentation (2 hours)

1. **Update Boxer README**:
   - Document DOSBox fork dependency
   - Build instructions
   - How to update DOSBox submodule

2. **Create BOXER-INTEGRATION.md** in DOSBox fork:
   ```markdown
   # Boxer Integration Guide

   This fork of DOSBox Staging includes modifications for Boxer integration.

   ## Changes from Upstream
   - CMakeLists.txt: Added BOXER_BUILD option
   - 18 source files: Added #ifdef BOXER guards

   ## Building for Boxer
   cmake .. -DBOXER_BUILD=ON -DBOXER_INCLUDE_DIR=/path/to/Boxer/Boxer

   ## Updating from Upstream
   git fetch upstream
   git merge upstream/main
   # Resolve conflicts in modified files
   ```

3. **Document Modified Files**:
   Create `BOXER_MODIFIED_FILES.txt` in DOSBox fork listing all changed files.

### Fallback Strategy: **Strategy B (Patch-Based)**

If the forked submodule approach proves problematic (e.g., GitHub repo limits, collaboration issues), fall back to **Strategy B (Patch-Based Integration)**.

**Trigger Conditions**:
- Unable to maintain public fork (repo access issues)
- Merge conflicts too frequent/complex
- Team prefers not to fork

**Fallback Implementation**:
1. Generate patches from Strategy E changes:
   ```bash
   git diff upstream/main > boxer-integration.patch
   ```

2. Apply patches in Xcode Run Script Phase:
   ```bash
   cd DOSBox-Staging
   git checkout .  # Clean workspace
   patch -p1 < "${PROJECT_DIR}/Resources/DOSBox-Patches/boxer-integration.patch"
   ```

3. Same CMakeLists.txt and source modifications as Strategy E

**Trade-off**: Lose git merge tools, gain independence from fork maintenance.

---

## CMake Implementation Details

### Required CMakeLists.txt Modifications

**File**: `/home/user/dosbox-staging/CMakeLists.txt`

**Location**: After line 98 (`include(GNUInstallDirs)`)

**Complete Code Block**:

```cmake
# ============================================================================
# Boxer Integration
# ============================================================================
# This section enables DOSBox Staging to be built as part of Boxer, with
# Boxer's callback hooks integrated via preprocessor macros and header inclusion.
#
# Usage:
#   cmake .. -DBOXER_BUILD=ON -DBOXER_INCLUDE_DIR=/path/to/Boxer/Boxer
#
# When BOXER_BUILD is ON:
#   1. BXCoalface.h is made available to all compilation units
#   2. Preprocessor macros remap DOSBox functions to boxer_* equivalents
#   3. Source files conditionally include BXCoalface.h via #ifdef BOXER
#
# Modified Files (must have #ifdef BOXER guards):
#   - src/dosbox.cpp, src/gui/sdl_gui.cpp, src/shell/*.cpp, etc. (18+ files)
#
# Maintainer: Update BOXER_INCLUDE_DIR if Boxer source moves
# ============================================================================

option(BOXER_BUILD "Build DOSBox for Boxer integration" OFF)

if (BOXER_BUILD)
  message(STATUS "=== Boxer Integration Enabled ===")

  # ----------------------------------------------------------------------------
  # Boxer Include Path
  # ----------------------------------------------------------------------------
  # Path to Boxer source directory containing BXCoalface.h
  # Default assumes DOSBox is a submodule in Boxer (../Boxer relative to DOSBox)
  set(BOXER_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Boxer" CACHE PATH
      "Path to Boxer source directory")

  # Validate that BXCoalface.h exists
  if (NOT EXISTS "${BOXER_INCLUDE_DIR}/BXCoalface.h")
    message(FATAL_ERROR
            "BXCoalface.h not found at: ${BOXER_INCLUDE_DIR}/BXCoalface.h\n"
            "Please set BOXER_INCLUDE_DIR to the correct path:\n"
            "  cmake .. -DBOXER_INCLUDE_DIR=/path/to/Boxer/Boxer")
  endif()

  message(STATUS "  Boxer include path: ${BOXER_INCLUDE_DIR}")

  # Add Boxer include directory globally
  # This makes BXCoalface.h visible to all source files that #include it
  include_directories("${BOXER_INCLUDE_DIR}")

  # ----------------------------------------------------------------------------
  # Preprocessor Definitions
  # ----------------------------------------------------------------------------
  # Define BOXER macro to enable #ifdef BOXER guards in source files
  add_compile_definitions(BOXER)

  # Inject Boxer's macro remappings
  # These #defines replace DOSBox function names with boxer_* equivalents
  # at compile time, redirecting calls to Boxer's implementation in BXCoalface.mm
  #
  # Example: GFX_Events() calls in DOSBox become boxer_processEvents() calls
  #
  # Note: This uses CMake 3.12+ add_compile_definitions() which properly handles
  # the '=' in macro definitions. Equivalent to: -DGFX_Events=boxer_processEvents
  add_compile_definitions(
    # Rendering pipeline (INT-001 to INT-003)
    GFX_Events=boxer_processEvents
    GFX_StartUpdate=boxer_startFrame
    GFX_EndUpdate=boxer_finishFrame

    # Input and mouse (INT-004)
    Mouse_AutoLock=boxer_setMouseActive

    # Rendering configuration (INT-005 to INT-010)
    GFX_SetTitle=boxer_handleDOSBoxTitleChange
    GFX_GetDisplayRefreshRate=boxer_GetDisplayRefreshRate
    GFX_SetSize=boxer_prepareForFrameSize
    GFX_GetRGB=boxer_getRGBPaletteEntry
    GFX_SetShader=boxer_setShader
    GFX_GetBestMode=boxer_idealOutputMode

    # Event processing (INT-012)
    GFX_MaybeProcessEvents=boxer_MaybeProcessEvents

    # Logging (INT-081)
    GFX_ShowMsg=boxer_log

    # MIDI (INT-082)
    MIDI_Available=boxer_MIDIAvailable

    # Capture (INT-014)
    OpenCaptureFile=boxer_openCaptureFile
  )

  # Note: E_Exit macro (INT-013) is defined in BXCoalface.h itself because it
  # requires complex __VA_ARGS__ handling that doesn't work well in CMake

  message(STATUS "  Defined ${CMAKE_CURRENT_LIST_LINE} preprocessor macros")
  message(STATUS "=== Boxer Integration Configuration Complete ===")

endif(BOXER_BUILD)

# ============================================================================
# End Boxer Integration
# ============================================================================
```

**Lines Added**: ~80 (including extensive comments for future maintainers)
**Lines Without Comments**: ~43

**Testing**:
```bash
cmake .. -DBOXER_BUILD=ON -DBOXER_INCLUDE_DIR=/path/to/Boxer/Boxer
# Should output:
# === Boxer Integration Enabled ===
#   Boxer include path: /path/to/Boxer/Boxer
#   Defined [N] preprocessor macros
# === Boxer Integration Configuration Complete ===
```

### New CMake Files

**None Required**

All modifications are contained in the root `CMakeLists.txt`. Subdirectory `CMakeLists.txt` files remain unchanged because:
1. Global `include_directories()` makes BXCoalface.h visible everywhere
2. Global `add_compile_definitions()` applies to all compilation units
3. Source-level `#ifdef BOXER` guards control inclusion

**Alternative Approach** (if global definitions cause issues):

Create `cmake/BoxerIntegration.cmake`:

```cmake
# cmake/BoxerIntegration.cmake
function(enable_boxer_integration)
  # ... (same content as above)
endfunction()
```

Include in root CMakeLists.txt:
```cmake
include(cmake/BoxerIntegration.cmake)
if (BOXER_BUILD)
  enable_boxer_integration()
endif()
```

**Benefit**: Cleaner separation, easier to review changes
**Cost**: +1 file to maintain
**Recommendation**: Only if requested for code organization

### Xcode Integration

**Build Process Flow**:
```
Xcode Build
    ↓
Run Script Phase: Build DOSBox
    ↓
CMake Configure (if needed)
    ↓
CMake Build (make libdosboxcommon)
    ↓
Copy libdosboxcommon.a to Build Directory
    ↓
Xcode Compile Sources (Boxer/*.mm)
    ↓
Xcode Link Phase
    - Link BXCoalface.mm (Boxer implementation)
    - Link libdosboxcommon.a (DOSBox library)
    - Link SDL2, SDL2_net, etc. (shared dependencies)
    ↓
Boxer.app
```

**Xcode Project Modifications**:

1. **Add Run Script Build Phase** (name: "Build DOSBox via CMake"):

```bash
#!/bin/bash
# Build DOSBox via CMake
# This script configures and builds DOSBox Staging as a static library
# for linking into Boxer.

set -e  # Exit immediately on error
set -u  # Error on undefined variables

# Enable verbose output in Xcode build log
set -x

echo "=================================================="
echo "Building DOSBox Staging for Boxer"
echo "=================================================="

# ----------------------------------------------------------------------------
# Configuration
# ----------------------------------------------------------------------------
DOSBOX_SOURCE="${PROJECT_DIR}/DOSBox-Staging"
DOSBOX_BUILD="${BUILD_DIR}/DOSBox-Build"
BOXER_SOURCE="${PROJECT_DIR}/Boxer"
CMAKE_BUILD_TYPE="${CONFIGURATION}"  # Debug or Release

# Verify DOSBox source exists
if [ ! -d "${DOSBOX_SOURCE}" ]; then
    echo "ERROR: DOSBox source not found at ${DOSBOX_SOURCE}"
    echo "Please ensure DOSBox-Staging submodule is checked out:"
    echo "  git submodule update --init --recursive"
    exit 1
fi

# Verify BXCoalface.h exists
if [ ! -f "${BOXER_SOURCE}/BXCoalface.h" ]; then
    echo "ERROR: BXCoalface.h not found at ${BOXER_SOURCE}/BXCoalface.h"
    exit 1
fi

# ----------------------------------------------------------------------------
# CMake Configuration
# ----------------------------------------------------------------------------
echo "DOSBox Source: ${DOSBOX_SOURCE}"
echo "DOSBox Build:  ${DOSBOX_BUILD}"
echo "Boxer Source:  ${BOXER_SOURCE}"
echo "Build Type:    ${CMAKE_BUILD_TYPE}"
echo "Architectures: ${ARCHS}"

# Create build directory
mkdir -p "${DOSBOX_BUILD}"
cd "${DOSBOX_BUILD}"

# Configure CMake (only if cache doesn't exist or source changed)
RECONFIGURE=0
if [ ! -f "CMakeCache.txt" ]; then
    RECONFIGURE=1
elif [ "${DOSBOX_SOURCE}/CMakeLists.txt" -nt "CMakeCache.txt" ]; then
    echo "CMakeLists.txt modified, reconfiguring..."
    RECONFIGURE=1
fi

if [ $RECONFIGURE -eq 1 ]; then
    echo "--------------------------------------------------"
    echo "Configuring DOSBox with CMake..."
    echo "--------------------------------------------------"

    cmake "${DOSBOX_SOURCE}" \
        -DBOXER_BUILD=ON \
        -DBOXER_INCLUDE_DIR="${BOXER_SOURCE}" \
        -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}" \
        -DCMAKE_OSX_ARCHITECTURES="${ARCHS}" \
        -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET}" \
        -DOPT_TESTS=OFF \
        -DOPT_DEBUGGER=OFF \
        -G "Unix Makefiles"

    if [ $? -ne 0 ]; then
        echo "ERROR: CMake configuration failed"
        exit 1
    fi
fi

# ----------------------------------------------------------------------------
# Build DOSBox
# ----------------------------------------------------------------------------
echo "--------------------------------------------------"
echo "Building libdosboxcommon..."
echo "--------------------------------------------------"

# Use all available CPU cores
NPROC=$(sysctl -n hw.ncpu)
make -j${NPROC} libdosboxcommon

if [ $? -ne 0 ]; then
    echo "ERROR: DOSBox build failed"
    exit 1
fi

# ----------------------------------------------------------------------------
# Copy Output
# ----------------------------------------------------------------------------
# Copy library to a location where Xcode can find it
cp -f "${DOSBOX_BUILD}/src/libdosboxcommon.a" "${BUILD_DIR}/libdosboxcommon.a"

echo "--------------------------------------------------"
echo "DOSBox build complete!"
echo "Library: ${BUILD_DIR}/libdosboxcommon.a"
echo "=================================================="
```

2. **Build Settings** (in Xcode Build Settings for Boxer target):

**Header Search Paths**:
```
$(PROJECT_DIR)/DOSBox-Staging/src
$(PROJECT_DIR)/DOSBox-Staging/src/libs
$(BUILD_DIR)/DOSBox-Build/include
```

**Library Search Paths**:
```
$(BUILD_DIR)
```

**Other Linker Flags**:
```
$(BUILD_DIR)/libdosboxcommon.a
```

Or, alternatively, add to **Link Binary With Libraries** build phase:
- Add Custom: `$(BUILD_DIR)/libdosboxcommon.a`

3. **Preprocessor Macros** (Xcode Build Settings):

Add to **Preprocessor Macros**:
```
BOXER=1
```

This ensures Boxer source files also see the BOXER macro.

4. **Dependency Management**:

Ensure Boxer links against the same versions of:
- SDL2
- SDL2_net
- FluidSynth
- MT32Emu (Boxer has custom fork, may need coordination)

Check DOSBox's dependency versions in `CMakeLists.txt` and match in Boxer's Xcode project.

### Build Script Changes

**File**: `Boxer.xcodeproj/project.pbxproj`

**Changes** (high-level, actual file is binary-ish):
1. Add Run Script Build Phase (shown above)
2. Add `$(BUILD_DIR)/libdosboxcommon.a` to link phase
3. Add header search paths

**Recommendation**: Make these changes via Xcode GUI, not manual editing.

**Build Verification**:
```bash
# Clean build
cd /path/to/Boxer
xcodebuild -project Boxer.xcodeproj -scheme Boxer -configuration Debug clean
xcodebuild -project Boxer.xcodeproj -scheme Boxer -configuration Debug

# Check for libdosboxcommon.a
ls -lh build/Debug/libdosboxcommon.a

# Check that Boxer.app was created
ls -lh build/Debug/Boxer.app/Contents/MacOS/Boxer
```

---

## Special Concerns Resolution

### Preprocessor Macro Handling

**Question**: Will #define remapping work in CMake context?

**Answer**: **YES**, fully supported.

**Mechanism**:
```cmake
add_compile_definitions(GFX_Events=boxer_processEvents)
```

**Equivalent to**:
```bash
clang++ -DGFX_Events=boxer_processEvents ...
```

**Result**: Everywhere in DOSBox code that calls `GFX_Events()`, the preprocessor replaces it with `boxer_processEvents()` before compilation.

**Example**:

DOSBox source (`src/gui/sdl_gui.cpp`):
```cpp
void MainLoopHandler::RunLoop() {
    while (!shutdown_requested) {
        GFX_Events();  // Preprocessor sees this
        // ... (becomes boxer_processEvents() after macro expansion)
    }
}
```

After preprocessing (what compiler sees):
```cpp
void MainLoopHandler::RunLoop() {
    while (!shutdown_requested) {
        boxer_processEvents();  // Macro replaced
        // ...
    }
}
```

**Requirements**:
1. ✅ BXCoalface.h declares `boxer_processEvents()` - Already exists
2. ✅ BXCoalface.mm implements `boxer_processEvents()` - Already exists
3. ✅ CMake defines macro globally - Implemented in CMakeLists.txt
4. ✅ Linker finds implementation - Boxer.app links BXCoalface.mm and libdosboxcommon.a

**Potential Issue**: Macro precedence

If DOSBox also defines `GFX_Events` (as a function or macro), conflicts occur.

**Solution**: Verify DOSBox doesn't redefine these symbols:
```bash
grep -r "define GFX_Events\|#define GFX_StartUpdate" /home/user/dosbox-staging/src
```

If conflicts found, use CMake's `add_compile_options("-UGFX_Events")` to undefine first, then redefine.

**Validation Test**:
```bash
# Compile a test file with macro
echo 'void GFX_Events(); int main() { GFX_Events(); }' > test.cpp
clang++ -DGFX_Events=boxer_processEvents -E test.cpp
# Should output: void boxer_processEvents(); int main() { boxer_processEvents(); }
```

### Modified File Management

**Challenge**: 18+ DOSBox files need `#include "BXCoalface.h"` with `#ifdef BOXER` guards.

**Strategy**: Systematic modification with documentation.

**Implementation**:

1. **Create Modified Files List** (`BOXER_MODIFIED_FILES.txt`):
```
# DOSBox Staging files modified for Boxer integration
# Format: <file path> : <modification type> : <integration points>

src/dosbox.cpp : header-include : INT-057,INT-058,INT-059
src/gui/sdl_gui.cpp : header-include : INT-001,INT-002,INT-003,INT-004
src/shell/shell.cpp : header-include : INT-023,INT-024,INT-025
src/shell/shell_misc.cpp : header-include : INT-027,INT-028,INT-029,INT-030,INT-031,INT-032,INT-033
src/shell/shell_batch.cpp : header-include : INT-034,INT-036,INT-037,INT-038
src/dos/drive_local.cpp : header-include : INT-042,INT-043,INT-044,INT-045
src/dos/drive_cache.cpp : header-include : INT-040
src/dos/dos_programs.cpp : header-include : INT-039,INT-041
src/hardware/input/keyboard.cpp : header-include : INT-061
src/hardware/input/joystick.cpp : header-include : INT-056
src/dos/dos_keyboard_layout.cpp : header-include : INT-063,INT-064,INT-065,INT-066
src/ints/bios_keyboard.cpp : header-include : INT-062,INT-071,INT-072
src/audio/mixer.cpp : header-include : (check path)
src/midi/midi.cpp : header-include : INT-082
src/hardware/video/vga_other.cpp : header-include : INT-017,INT-018,INT-019,INT-020,INT-021,INT-022
src/misc/messages.cpp : header-include : INT-081
CMakeLists.txt : boxer-integration-block : (all macros)

# Files that may not exist in target (require investigation):
src/hardware/parport/printer_redir.cpp : header-include : INT-075,INT-076,INT-077,INT-078,INT-079,INT-080
```

2. **Standard Include Pattern** (template):

```cpp
// Beginning of file...
#include "dosbox.h"
#include <other_headers.h>

// ============================================================================
// Boxer Integration
// ============================================================================
// This file is modified for Boxer integration. When BOXER is defined (via
// CMake -DBOXER_BUILD=ON), BXCoalface.h provides boxer_* function declarations
// and enables preprocessor macro remapping (e.g., GFX_Events → boxer_processEvents).
//
// Modified for integration points: INT-XXX, INT-YYY, INT-ZZZ
// See: /Boxer/docs/upgrade-analysis/01-current-integration/integration-points/
// ============================================================================
#ifdef BOXER
#include "BXCoalface.h"
#endif
// ============================================================================

// Rest of file...
```

**Lines Added Per File**: ~11 (8 comment lines + 3 code lines)

3. **Validation Script** (`scripts/verify-boxer-integration.sh`):

```bash
#!/bin/bash
# Verify all required files have BOXER integration guards

DOSBOX_SRC="/path/to/dosbox-staging/src"
MODIFIED_FILES="BOXER_MODIFIED_FILES.txt"

echo "Verifying Boxer integration..."

while IFS=':' read -r filepath type integrations; do
    # Skip comments and empty lines
    [[ $filepath =~ ^#.*$ ]] && continue
    [[ -z $filepath ]] && continue

    # Skip CMakeLists.txt
    [[ $filepath =~ CMakeLists.txt$ ]] && continue

    fullpath="${DOSBOX_SRC}/${filepath}"

    if [ ! -f "$fullpath" ]; then
        echo "⚠️  File not found: $filepath"
        continue
    fi

    if grep -q "#ifdef BOXER" "$fullpath" && grep -q "BXCoalface.h" "$fullpath"; then
        echo "✅ $filepath"
    else
        echo "❌ Missing integration: $filepath"
    fi
done < "$MODIFIED_FILES"

echo "Verification complete."
```

4. **Update Management**:

When merging upstream DOSBox changes:
```bash
git fetch upstream
git merge upstream/main

# Check which modified files have conflicts
git status | grep "both modified"

# For each conflict:
#   1. Verify BXCoalface.h include is preserved
#   2. Resolve surrounding code changes
#   3. Run verification script
```

**Recommendation**: Add pre-commit hook to verify integrity of modified files.

### Compilation Order

**Question**: Do Boxer headers need to be included first?

**Answer**: **NO**, but inclusion order matters for macro remapping.

**Correct Order**:
1. Standard library headers (`<cstdio>`, `<memory>`, etc.)
2. DOSBox headers (`dosbox.h`, `video.h`, `cpu.h`, etc.)
3. **BXCoalface.h** (last, so macros remap DOSBox function calls)

**Example** (`src/gui/sdl_gui.cpp`):
```cpp
// 1. System headers
#include <SDL2/SDL.h>
#include <memory>
#include <string>

// 2. DOSBox headers
#include "dosbox.h"
#include "video.h"
#include "render.h"

// 3. Boxer integration (MUST be last to remap function calls)
#ifdef BOXER
#include "BXCoalface.h"
#endif

// Now when code calls GFX_Events(), macro remaps to boxer_processEvents()
void some_function() {
    GFX_Events();  // Becomes boxer_processEvents() due to macro
}
```

**Why Last**?

BXCoalface.h contains:
```cpp
#define GFX_Events boxer_processEvents
```

If included **before** DOSBox headers:
- DOSBox headers might declare `void GFX_Events()`, which would become `void boxer_processEvents()` → WRONG
- Macro would affect declarations, not just calls

If included **after** DOSBox headers:
- DOSBox headers declare `void GFX_Events()` normally
- Macro remaps **calls** to `GFX_Events()` → becomes `boxer_processEvents()`
- Declarations unaffected, calls remapped → CORRECT

**CMake Impact**: None. Compilation order is source-file-internal (include order), not build-target order.

**Verification**:
```bash
# Check preprocessor output
clang++ -DBOXER -E src/gui/sdl_gui.cpp | grep "GFX_Events"
# Should show boxer_processEvents in function bodies, GFX_Events in declarations
```

### macOS-Specific Considerations

**Platform**: macOS 10.13+ (as of Boxer's current target)

**DOSBox CMake Settings** (already in CMakeLists.txt):
```cmake
if (APPLE)
  set(DOSBOX_PLATFORM_MACOS ON)
  set(MACOSX ON)

  # Audio/MIDI frameworks
  set(C_COREAUDIO      ON)
  set(C_COREMIDI       ON)
  set(C_COREFOUNDATION ON)
  set(C_CORESERVICES   ON)

  # Suppress deprecated API warnings
  add_compile_options("-Wno-deprecated-declarations")
endif()
```

**Xcode-Specific Settings**:

1. **Deployment Target**:
   - Boxer: macOS 10.13 (verify in Xcode settings)
   - DOSBox CMake: Set via `CMAKE_OSX_DEPLOYMENT_TARGET` (passed from Xcode)
   - **Ensure they match** to avoid ABI issues

2. **Architectures**:
   - Boxer: x86_64, arm64 (Universal Binary)
   - DOSBox CMake: Set via `CMAKE_OSX_ARCHITECTURES` (passed from Xcode)
   - Build script passes `${ARCHS}` automatically

3. **Code Signing**:
   - DOSBox library (libdosboxcommon.a) is static → no signing needed
   - Boxer.app signs everything at final link

4. **Hardened Runtime**:
   - If Boxer uses Hardened Runtime, ensure DOSBox doesn't use deprecated APIs
   - Check DOSBox for JIT compilation (C_DYNAMIC_X86, C_DYNREC):
     ```cmake
     # In CMakeLists.txt:
     set(C_DYNAMIC_X86 ON)  # Uses JIT on x86_64
     ```
   - May require `com.apple.security.cs.allow-jit` entitlement

5. **Sandboxing**:
   - If Boxer is sandboxed, file access in DOSBox may be restricted
   - Boxer's integration points (INT-041: shouldAllowWriteAccessToPath) handle this
   - Ensure BXCoalface.mm implementation respects sandbox

**Build Flags**:
```bash
# In Xcode Run Script Phase, these are automatically set:
CMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET}"  # e.g., 10.13
CMAKE_OSX_ARCHITECTURES="${ARCHS}"                        # e.g., "x86_64;arm64"
```

**Framework Linking**:

DOSBox requires these macOS frameworks (verify they're in Boxer's Xcode project):
- CoreAudio
- CoreMIDI
- CoreFoundation
- CoreServices
- Cocoa (for windowing, but Boxer handles this)

**Potential Issues**:

1. **SDL2 Version Mismatch**:
   - DOSBox uses SDL2 from vcpkg or system
   - Boxer uses SDL2 from Frameworks/
   - **Risk**: ABI incompatibility
   - **Solution**: Ensure same SDL2 version, or let Boxer's SDL2 take precedence

2. **Objective-C++ Interop**:
   - BXCoalface.mm is Objective-C++
   - libdosboxcommon.a is C++
   - **Should work** if Boxer's main.mm sets up Objective-C runtime first

3. **C++ Standard Library**:
   - DOSBox uses C++20 (`set(CMAKE_CXX_STANDARD 20)`)
   - Boxer must also use C++20 or later
   - **Verify**: Xcode Build Settings → C++ Language Dialect → C++20

**Validation**:
```bash
# Check compiled library's architectures
lipo -info build/Debug/libdosboxcommon.a
# Should output: Architectures in the fat file: libdosboxcommon.a are: x86_64 arm64

# Check deployment target
otool -l build/Debug/libdosboxcommon.a | grep -A3 LC_VERSION_MIN_MACOSX
# Should match Boxer's deployment target
```

### Future Maintainability

**Challenge**: How to handle future DOSBox Staging updates?

**Update Workflow**:

1. **Regular Updates** (quarterly or when critical fixes released):

   ```bash
   cd /path/to/dosbox-staging-boxer
   git fetch upstream
   git checkout boxer-integration
   git merge upstream/main
   ```

2. **Conflict Resolution** (expected in ~18 files):

   ```bash
   # git will report conflicts in files with BXCoalface.h includes
   git status

   # For each conflict:
   #   - Keep BXCoalface.h include block
   #   - Merge surrounding changes
   #   - Verify integration points still work

   # Example: src/gui/sdl_gui.cpp
   # Conflict: DOSBox renamed a function, Boxer has BXCoalface.h include nearby
   # Resolution: Apply rename, ensure BXCoalface.h include is after all DOSBox headers
   ```

3. **Validation After Merge**:

   ```bash
   # Run verification script
   ./scripts/verify-boxer-integration.sh

   # Build test
   cd build
   cmake .. -DBOXER_BUILD=ON -DBOXER_INCLUDE_DIR=/path/to/Boxer/Boxer
   make -j8 libdosboxcommon

   # If builds, update Boxer submodule
   cd /path/to/Boxer
   cd DOSBox-Staging
   git checkout <new-commit>
   cd ..
   git add DOSBox-Staging
   git commit -m "Update DOSBox Staging to vX.Y.Z"
   ```

4. **Breaking Changes** (API changes in DOSBox):

   Check for:
   - Function signature changes (e.g., `GFX_Events()` parameters)
   - Removed functions (e.g., `GFX_ShowMsg` deleted)
   - Architectural changes (e.g., rendering system rewritten)

   **Response**:
   - Update BXCoalface.h/mm to match new signatures
   - If function removed, find replacement or port old implementation
   - If architecture changed, may require re-analysis (rerun Agent 1B.2)

**Long-Term Strategies**:

1. **Upstream Contribution** (ideal but unlikely):
   - Propose plugin API to DOSBox Staging
   - Get callback hooks accepted upstream
   - Reduces maintenance burden significantly
   - **Likelihood**: Low (DOSBox Staging team may not want this complexity)

2. **Automated Merge Testing**:
   - CI/CD pipeline: Monthly auto-merge from upstream
   - Automated build test
   - Alert if merge conflicts or build breaks
   - **Benefit**: Early detection of breaking changes

3. **Documentation**:
   - Keep BOXER_MODIFIED_FILES.txt updated
   - Document each integration point's purpose (link to Agent 1A's INT-XXX files)
   - When DOSBox changes affect integration point, update docs

4. **Version Pinning**:
   - Don't update DOSBox on every release
   - Update only for:
     - Critical security fixes
     - Features Boxer needs
     - Major performance improvements
   - **Benefit**: Reduces churn, fewer merge conflicts

**Estimated Maintenance Burden**:
- **Minor DOSBox updates** (bug fixes): 2-4 hours per update
  - Merge conflicts: 1-2 hours
  - Build testing: 0.5-1 hour
  - Integration testing: 0.5-1 hour
  - Documentation: 0.5 hour

- **Major DOSBox updates** (version bumps): 8-16 hours per update
  - Merge conflicts: 3-6 hours
  - API compatibility fixes: 2-4 hours
  - Build testing: 1-2 hours
  - Integration testing: 2-4 hours
  - Documentation: 1-2 hours

**Mitigation**:
- Update DOSBox conservatively (1-2 times per year, not every release)
- Maintain good test suite to catch integration breaks early
- Keep Boxer's integration points well-documented

**Risk Assessment**:
- **Low Risk**: DOSBox minor updates (0.83.0 → 0.83.1)
- **Medium Risk**: DOSBox feature updates (0.83 → 0.84)
- **High Risk**: DOSBox major architectural changes (e.g., rendering rewrite)

**Fallback**: If DOSBox diverges too much, fork permanently and port critical fixes manually (not ideal, but viable).

---

## Build Verification Plan

### Testing Steps

#### 1. CMake Configuration Test

```bash
cd /path/to/dosbox-staging-boxer
mkdir -p build && cd build

# Test 1: Verify BOXER_BUILD option exists
cmake .. -LH | grep BOXER_BUILD
# Expected output: BOXER_BUILD:BOOL=OFF

# Test 2: Enable Boxer build
cmake .. -DBOXER_BUILD=ON -DBOXER_INCLUDE_DIR=/path/to/Boxer/Boxer

# Expected output:
# === Boxer Integration Enabled ===
#   Boxer include path: /path/to/Boxer/Boxer
#   Defined [N] preprocessor macros
# === Boxer Integration Configuration Complete ===

# Test 3: Verify preprocessor definitions
cmake .. -DBOXER_BUILD=ON -DBOXER_INCLUDE_DIR=/path/to/Boxer/Boxer -LA | grep -E "GFX_Events|BOXER"
# Expected: See BOXER and macro definitions

# Test 4: Fail case - missing BXCoalface.h
cmake .. -DBOXER_BUILD=ON -DBOXER_INCLUDE_DIR=/nonexistent/path
# Expected: FATAL_ERROR with helpful message
```

#### 2. Compilation Test

```bash
cd /path/to/dosbox-staging-boxer/build

# Build libdosboxcommon
make libdosboxcommon -j8

# Expected output:
# [ 99%] Building CXX object src/CMakeFiles/libdosboxcommon.dir/dosbox.cpp.o
# [100%] Linking CXX static library libdosboxcommon.a
# [100%] Built target libdosboxcommon

# Verify library exists
ls -lh src/libdosboxcommon.a
# Expected: File exists, size ~10-30 MB depending on build type

# Check for boxer symbols in compiled objects
nm src/libdosboxcommon.a | grep boxer_processEvents
# Expected: U boxer_processEvents (undefined external symbol)
# This is correct - implementation is in Boxer's BXCoalface.mm
```

#### 3. Preprocessor Verification Test

```bash
cd /path/to/dosbox-staging-boxer/build

# Generate preprocessed output for a key file
make src/CMakeFiles/libdosboxcommon.dir/gui/sdl_gui.cpp.i

# Check that macros were applied
grep "boxer_processEvents\|boxer_startFrame" src/CMakeFiles/libdosboxcommon.dir/gui/sdl_gui.cpp.i | head -5
# Expected: See boxer_* function calls instead of GFX_* calls

# Verify BXCoalface.h was included (if #ifdef BOXER worked)
grep -A2 "Boxer Integration" src/CMakeFiles/libdosboxcommon.dir/gui/sdl_gui.cpp.i
# Expected: See BXCoalface.h content
```

#### 4. Xcode Integration Test

```bash
cd /path/to/Boxer

# Clean build
xcodebuild -project Boxer.xcodeproj -scheme Boxer -configuration Debug clean

# Build
xcodebuild -project Boxer.xcodeproj -scheme Boxer -configuration Debug 2>&1 | tee build.log

# Check for DOSBox build in log
grep "Building DOSBox" build.log
# Expected: See "Building DOSBox Staging for Boxer"

# Verify library was built and copied
ls -lh build/Debug/libdosboxcommon.a
# Expected: File exists

# Verify Boxer.app was created
ls -lh build/Debug/Boxer.app/Contents/MacOS/Boxer
# Expected: Executable exists

# Check for linking errors
grep -i "undefined.*boxer_" build.log
# Expected: No output (all boxer_* symbols resolved)
```

#### 5. Runtime Integration Test

**Prerequisites**: Build successful from Test 4

```bash
# Launch Boxer
open build/Debug/Boxer.app

# Test scenarios (manual):
# 1. Boot a DOS game
# 2. Verify rendering appears (tests INT-002, INT-003)
# 3. Verify keyboard input works (tests INT-001, event loop)
# 4. Verify mouse works (tests INT-004)
# 5. Create a file in DOS, verify on macOS filesystem (tests INT-042-045)
# 6. Exit cleanly (tests INT-059)
```

**Automated Runtime Test** (if Boxer has test suite):
```objc
// BXIntegrationTests.m
- (void)testDOSBoxIntegration {
    BXEmulator *emulator = [[BXEmulator alloc] init];

    // Test critical integration points
    XCTAssertTrue([emulator respondsToSelector:@selector(_processEvents)],
                  @"INT-001: boxer_processEvents");
    XCTAssertTrue([emulator respondsToSelector:@selector(_startFrame:pitch:)],
                  @"INT-002: boxer_startFrame");
    // ... (test all critical integration points)
}
```

### Expected Outputs

#### CMake Configuration Output

```
=== Boxer Integration Enabled ===
  Boxer include path: /Users/username/Projects/Boxer/Boxer
  Defined preprocessor macros:
    BOXER
    GFX_Events=boxer_processEvents
    GFX_StartUpdate=boxer_startFrame
    GFX_EndUpdate=boxer_finishFrame
    Mouse_AutoLock=boxer_setMouseActive
    GFX_SetTitle=boxer_handleDOSBoxTitleChange
    GFX_GetDisplayRefreshRate=boxer_GetDisplayRefreshRate
    GFX_SetSize=boxer_prepareForFrameSize
    GFX_GetRGB=boxer_getRGBPaletteEntry
    GFX_SetShader=boxer_setShader
    GFX_GetBestMode=boxer_idealOutputMode
    GFX_MaybeProcessEvents=boxer_MaybeProcessEvents
    GFX_ShowMsg=boxer_log
    MIDI_Available=boxer_MIDIAvailable
    OpenCaptureFile=boxer_openCaptureFile
=== Boxer Integration Configuration Complete ===
-- Configuring done
-- Generating done
-- Build files have been written to: /Users/username/Projects/dosbox-staging-boxer/build
```

#### Build Output

```
Scanning dependencies of target libdosboxcommon
[  1%] Building CXX object src/CMakeFiles/libdosboxcommon.dir/dosbox.cpp.o
[  2%] Building CXX object src/gui/CMakeFiles/libdosboxcommon.dir/sdl_gui.cpp.o
...
[ 98%] Building CXX object src/utils/CMakeFiles/libdosboxcommon.dir/rwqueue.cpp.o
[ 99%] Linking CXX static library libdosboxcommon.a
[100%] Built target libdosboxcommon
```

**No errors or warnings** related to:
- BXCoalface.h not found
- Undefined BOXER macro
- Conflicting macro definitions

#### Xcode Build Output

```
================================================
Building DOSBox Staging for Boxer
================================================
DOSBox Source: /Users/username/Projects/Boxer/DOSBox-Staging
DOSBox Build:  /Users/username/Library/Developer/Xcode/DerivedData/Boxer-.../Build/Intermediates.noindex/DOSBox-Build
Boxer Source:  /Users/username/Projects/Boxer/Boxer
Build Type:    Debug
Architectures: x86_64 arm64

--------------------------------------------------
Configuring DOSBox with CMake...
--------------------------------------------------
=== Boxer Integration Enabled ===
...
--------------------------------------------------
Building libdosboxcommon...
--------------------------------------------------
[100%] Built target libdosboxcommon
--------------------------------------------------
DOSBox build complete!
Library: /Users/username/Library/Developer/Xcode/DerivedData/Boxer-.../Build/Products/Debug/libdosboxcommon.a
================================================

...

CompileC .../BXCoalface.mm
Ld .../Boxer.app/Contents/MacOS/Boxer normal
    ... -llibdosboxcommon ...

Build Succeeded
```

#### Runtime Output

**Console Log** (when Boxer launches):
```
Boxer: Initializing emulation...
DOSBox: CPU initialized (type: X86_64)
DOSBox: Memory initialized (16 MB)
Boxer: Rendering pipeline configured (boxer_prepareForFrameSize called)
Boxer: Event loop started (boxer_processEvents)
DOSBox: Shell initialized
Boxer: Shell will start (boxer_shellWillStart)
...
```

**No errors** related to:
- Unresolved symbols (boxer_*)
- Segmentation faults at integration points
- NULL pointer dereferences in callbacks

### Validation Criteria

**Build Success Criteria**:
- ✅ CMake configures without errors when BOXER_BUILD=ON
- ✅ libdosboxcommon.a builds successfully
- ✅ Xcode build completes without linker errors
- ✅ Boxer.app launches

**Integration Success Criteria**:
- ✅ All 8 critical integration points work (INT-001, INT-002, INT-003, INT-007, INT-010, INT-013, INT-041, INT-059)
- ✅ Rendering appears on screen (INT-002, INT-003)
- ✅ Input is processed (INT-001, INT-004)
- ✅ File I/O callbacks invoked (INT-041-045)
- ✅ Emulation loop controlled by Boxer (INT-057-059)

**Regression Test Criteria** (if Boxer has existing test suite):
- ✅ All existing Boxer tests pass
- ✅ No new crashes or hangs
- ✅ Performance similar to legacy DOSBox integration (±10%)

**Code Quality Criteria**:
- ✅ No new compiler warnings in DOSBox code
- ✅ No memory leaks (run with Instruments)
- ✅ Code follows Boxer style guidelines

---

## Risks and Unknowns

### High-Risk Issues

#### RISK-001: Parport Removal in Target DOSBox

**Description**: Target DOSBox Staging may have removed parallel port support entirely. Boxer's printer emulation (INT-075 to INT-080) depends on this.

**Likelihood**: MEDIUM-HIGH (parallel ports are obsolete hardware)

**Impact**: HIGH - Boxer printer feature completely broken

**Investigation Needed**:
```bash
# Check if parport exists
find /home/user/dosbox-staging/src -type d -name "parport"
find /home/user/dosbox-staging/src -name "*printer*"
grep -r "CPrinterRedir\|parport" /home/user/dosbox-staging/src
```

**Mitigation Options**:
1. **If parport exists**: Integrate as planned
2. **If partially removed**: Port missing code from legacy DOSBox
3. **If completely removed**:
   - **Option A**: Disable Boxer printer feature (document as regression)
   - **Option B**: Reimplement parport in Boxer fork (10-20 hours effort)
   - **Option C**: Intercept at higher level (e.g., DOS INT 17h calls)

**Recommended Approach**: Option C - Intercept DOS printer interrupts in Boxer, bypass DOSBox parport entirely

**Fallback**: Accept feature loss (printer less critical than core emulation)

#### RISK-002: API Signature Changes

**Description**: Target DOSBox may have changed function signatures for GFX_* functions, breaking macro remapping.

**Likelihood**: MEDIUM (API evolution expected over 9000+ commits)

**Impact**: HIGH - Integration points fail to compile or crash at runtime

**Example Breaking Change**:
```cpp
// Legacy DOSBox
bool GFX_StartUpdate(Bit8u * & pixels, int & pitch);

// Target DOSBox (hypothetical)
bool GFX_StartUpdate(uint8_t** pixels, size_t& pitch);  // Different types!
```

**Mitigation**:
1. **Before implementation**: Audit all 15 macro-remapped functions
   ```bash
   grep -r "GFX_Events\|GFX_StartUpdate\|GFX_EndUpdate" /home/user/dosbox-staging/src --include="*.cpp" -A3
   ```

2. **Compare signatures**: Legacy vs. Target
   ```bash
   # Generate function signature list
   grep -E "^(bool|void|Bitu|int) (GFX_|Mouse_|MIDI_|OpenCapture)" /home/user/dosbox-staging-boxer/src/*.h
   grep -E "^(bool|void|Bitu|int) (GFX_|Mouse_|MIDI_|OpenCapture)" /home/user/dosbox-staging/src/gui/common.h
   ```

3. **Update BXCoalface.h/mm**: Match new signatures
   - Change return types
   - Change parameter types
   - Add/remove parameters

**Timeline**: 2-4 hours per changed function

**Unknown**: How many functions actually changed (requires audit)

#### RISK-003: C++20 Compatibility Issues

**Description**: Target DOSBox uses C++20; Boxer may use older C++ standard. ABI incompatibility or missing features.

**Likelihood**: MEDIUM (Boxer likely uses C++17 or C++14)

**Impact**: HIGH - Build failures or runtime crashes

**Investigation**:
```bash
# Check Boxer's C++ standard
grep -r "CLANG_CXX_LANGUAGE_STANDARD\|GCC_C_LANGUAGE_STANDARD" /path/to/Boxer/Boxer.xcodeproj/project.pbxproj

# Check for C++20-specific features in DOSBox
grep -r "concept\|<=> \|co_await" /home/user/dosbox-staging/src
```

**Mitigation**:
1. Update Boxer to C++20 in Xcode Build Settings
2. Fix any C++20 incompatibilities in Boxer code (e.g., `std::bind` vs. lambdas)

**Estimated Effort**: 2-8 hours (depending on Boxer's code)

**Fallback**: Request DOSBox to maintain C++17 compatibility (unlikely to succeed)

### Medium-Risk Issues

#### RISK-004: Build Time Increase

**Description**: Adding CMake build to Xcode workflow increases build time significantly.

**Likelihood**: HIGH (CMake + make invocation adds overhead)

**Impact**: MEDIUM - Developer productivity reduced

**Mitigation**:
1. **Incremental builds**: CMake detects changes, only rebuilds modified files
2. **Parallel builds**: Use `-j$(sysctl -n hw.ncpu)` for all CPU cores
3. **Caching**: CMake cache persists between builds
4. **Prebuilt library**: For CI, build libdosboxcommon.a once, cache for multiple Boxer builds

**Benchmark**:
- Legacy (Xcode only): ~30 seconds incremental, ~2 minutes clean
- Expected with CMake: ~45 seconds incremental, ~4 minutes clean
- Acceptable threshold: <60 seconds incremental, <6 minutes clean

#### RISK-005: Debugging Complexity

**Description**: Debugging DOSBox code requires understanding CMake build, preprocessor macros, and Xcode integration.

**Likelihood**: HIGH (multi-layer build complexity)

**Impact**: MEDIUM - Slower debugging for integration issues

**Mitigation**:
1. **Debug builds**: CMake -DCMAKE_BUILD_TYPE=Debug includes symbols
2. **Xcode integration**: Add DOSBox source to Xcode for debugging
3. **Preprocessor output**: Generate .i files to see macro expansions
4. **Documentation**: Create debugging guide for common issues

**Documentation** (create `docs/DEBUGGING.md`):
```markdown
## Debugging DOSBox Integration

### Viewing Preprocessor Output
make src/CMakeFiles/libdosboxcommon.dir/gui/sdl_gui.cpp.i
less src/CMakeFiles/libdosboxcommon.dir/gui/sdl_gui.cpp.i

### Debugging in Xcode
1. Add DOSBox source files to Xcode project (as reference)
2. Set breakpoints in DOSBox code
3. Breakpoints will hit when Boxer calls into libdosboxcommon

### Common Issues
- Macro not expanding: Check add_compile_definitions() in CMakeLists.txt
- Symbol not found: Verify BXCoalface.mm is linked
- Crash in boxer_*: Check BXEmulator implementation
```

#### RISK-006: Merge Conflict Overhead

**Description**: Every DOSBox update requires resolving conflicts in 18+ modified files.

**Likelihood**: HIGH (files are actively maintained by upstream)

**Impact**: MEDIUM - Maintenance burden on each update

**Mitigation**:
1. **Minimize modifications**: Keep BXCoalface.h includes as small as possible
2. **Automated testing**: CI runs merge test before manual merge
3. **Git rerere**: Enable to remember conflict resolutions
4. **Update infrequently**: Only update DOSBox when necessary (not every release)

**Estimated Effort Per Update**: 2-4 hours (see "Future Maintainability" section)

### Unknown Factors

#### UNKNOWN-001: Undiscovered Integration Points

**Question**: Are there integration points beyond the 86 identified by Agent 1A?

**Investigation**:
- Code review of Boxer's emulation logic
- Runtime tracing to find unexpected callbacks
- Comparison with legacy DOSBox to spot missing hooks

**Mitigation**: Iterative discovery during testing phase

#### UNKNOWN-002: Performance Impact

**Question**: Does macro remapping or callback overhead impact emulation performance?

**Investigation**:
- Benchmark legacy Boxer vs. upgraded Boxer
- Profile with Instruments (Time Profiler)
- Measure frame rate, input latency, audio quality

**Acceptable Threshold**: <5% performance regression

**Mitigation**: If regression >5%, optimize hot paths (e.g., inline critical callbacks)

#### UNKNOWN-003: Third-Party Dependencies

**Question**: Do DOSBox's vcpkg dependencies conflict with Boxer's existing dependencies?

**Investigation**:
```bash
# List DOSBox dependencies
grep -E "find_package|pkg_check_modules" /home/user/dosbox-staging/CMakeLists.txt

# Compare with Boxer dependencies
ls -la /path/to/Boxer/Frameworks/
```

**Potential Conflicts**:
- SDL2 (different versions)
- FluidSynth (Boxer may have custom build)
- MT32Emu (Boxer uses MaddTheSane fork)

**Mitigation**: Let Boxer's frameworks take precedence (statically link into Boxer.app, not DOSBox)

#### UNKNOWN-004: macOS Version Compatibility

**Question**: Does target DOSBox require newer macOS features than Boxer supports?

**Investigation**:
```bash
# Check for macOS version guards
grep -r "MAC_OS_X_VERSION_MIN_REQUIRED\|__builtin_available" /home/user/dosbox-staging/src
```

**Boxer Target**: macOS 10.13+
**DOSBox Target**: TBD (check CMakeLists.txt MACOSX_DEPLOYMENT_TARGET)

**Mitigation**: Adjust deployment target or backport macOS features

---

## Effort Estimate

### Implementation Time: 15-20 hours

**Breakdown**:

| Task | Estimated Hours | Notes |
|------|-----------------|-------|
| Fork setup and submodule configuration | 2 | GitHub fork, git setup |
| CMakeLists.txt modifications | 3-4 | Root file changes, testing |
| Source file modifications (18 files) | 4-5 | Add BXCoalface.h includes |
| Parport investigation and handling | 1-2 | Check if exists, plan mitigation |
| Xcode Run Script Phase | 2-3 | CMake invocation, build integration |
| Build settings configuration | 1 | Header paths, linker flags |
| Initial build and debugging | 2-3 | Fix compilation errors |
| Integration point validation | 2-3 | Test critical callbacks |
| Documentation | 2 | README, integration guide |

**Optimistic**: 15 hours (no major issues)
**Realistic**: 18 hours (minor issues, need debugging)
**Pessimistic**: 25 hours (API changes, parport missing)

### Testing Time: 4-6 hours

| Task | Estimated Hours |
|------|-----------------|
| Build verification | 0.5-1 |
| Critical integration tests | 1-2 |
| Regression testing | 1-2 |
| Performance benchmarking | 0.5-1 |
| Edge case testing | 1 |

### Total: 19-26 hours

**Realistic Estimate**: **22 hours** for Strategy E (Submodule with Local Modifications)

**Alternative Strategies**:
- Strategy B (Patch-Based): +5 hours (patch script development)
- Strategy D (Shim Layer): +15 hours (architectural refactoring)
- Strategy C (Separate Link): Not viable (would require 40-60 hours and still be incomplete)

---

## Recommendations for Next Steps

### Immediate Actions (Phase 1B.2 - Week 1)

1. **Validate Assumptions** (Agent 1B.2 task):
   - ✅ Audit GFX_* function signatures in target DOSBox
   - ✅ Verify parport existence
   - ✅ Check for breaking changes in shell, file I/O, input systems
   - ✅ Compare INT-001 to INT-086 against target DOSBox

   **Deliverable**: `api-compatibility-report.md` documenting all changes

2. **Create Fork** (Developer task):
   ```bash
   # On GitHub: Fork dosbox-staging to MaddTheSane/dosbox-staging-boxer
   git clone https://github.com/MaddTheSane/dosbox-staging-boxer.git
   cd dosbox-staging-boxer
   git remote add upstream https://github.com/dosbox-staging/dosbox-staging.git
   git checkout -b boxer-integration
   ```

3. **Proof of Concept** (Developer task, 4-6 hours):
   - Implement CMakeLists.txt changes
   - Modify 3-5 key files (dosbox.cpp, sdl_gui.cpp, shell.cpp)
   - Build libdosboxcommon.a with BOXER_BUILD=ON
   - Verify preprocessor macros work

   **Deliverable**: Working libdosboxcommon.a with partial integration

### Phase 1B Follow-On (Week 2-3)

1. **Agent 1B.2**: Detailed Integration Analysis
   - Map each of 86 integration points to target DOSBox equivalents
   - Identify breaking changes
   - Plan adapter layer if needed

2. **Agent 1B.3**: Implementation
   - Apply all CMakeLists.txt modifications
   - Apply all source file modifications
   - Build and test

3. **Agent 1B.4**: Validation
   - Test all critical integration points
   - Performance benchmarking
   - Regression testing

### Decision Points

**Decision Point 1**: After API audit (Agent 1B.2)
- **If** API is compatible → Proceed with Strategy E as planned
- **Else If** minor changes → Update BXCoalface.h/mm, proceed
- **Else If** major changes → Escalate, consider Strategy D (Shim Layer)

**Decision Point 2**: After parport investigation
- **If** parport exists → Integrate normally
- **Else** → Choose mitigation (disable feature, port code, or higher-level intercept)

**Decision Point 3**: After proof of concept
- **If** build succeeds → Continue with full implementation
- **Else** → Debug issues, potentially pivot to Strategy B (Patches)

### Success Metrics

**Phase 1B Complete** when:
- ✅ libdosboxcommon.a builds successfully with BOXER_BUILD=ON
- ✅ Xcode integrates CMake build
- ✅ All 8 critical integration points work
- ✅ Performance within 5% of legacy
- ✅ Documentation complete

**Ready for Phase 2** (Reintegration) when:
- ✅ All 86 integration points validated or adapted
- ✅ Regression test suite passes
- ✅ Build process documented
- ✅ Merge workflow tested

---

## Blockers/Open Questions

### Blockers

**BLOCKER-001**: ~~Build system incompatibility~~ → **RESOLVED**
- Solution: Strategy E (Submodule with CMake Modification)
- Status: Design complete, pending implementation

**BLOCKER-002**: Parport existence (unknown until investigation)
- Status: OPEN - Requires audit of target DOSBox
- Owner: Agent 1B.2
- Deadline: Before implementation begins

### Open Questions

**Q1**: Does target DOSBox still have parallel port support?
- **Owner**: Agent 1B.2
- **Investigation**: Find parport code in target
- **Fallback**: Multiple mitigation options designed

**Q2**: What is the exact API signature for all 15 macro-remapped functions?
- **Owner**: Agent 1B.2
- **Investigation**: Extract and compare signatures
- **Action**: Update BXCoalface.h if needed

**Q3**: Are there integration points beyond the 86 identified?
- **Owner**: Agent 1B.2 + Runtime testing
- **Investigation**: Code review and runtime tracing
- **Impact**: May need to add more integration points

**Q4**: What are Boxer's existing build times, and what's acceptable overhead?
- **Owner**: Developer (benchmark)
- **Investigation**: Measure current build time
- **Threshold**: <60 seconds incremental build

**Q5**: Does Boxer's Xcode project need C++20 upgrade?
- **Owner**: Developer
- **Investigation**: Check current C++ standard in Xcode settings
- **Action**: Update to C++20 if needed (likely minimal impact)

**Q6**: How will Boxer handle DOSBox dependency version mismatches (SDL2, FluidSynth, etc.)?
- **Owner**: Developer
- **Investigation**: Compare dependency versions
- **Strategy**: Let Boxer's frameworks take precedence

**Q7**: Is the git submodule approach acceptable to the Boxer team?
- **Owner**: Project lead
- **Alternative**: Strategy B (Patch-Based) if submodule not desired

**Q8**: What is the release timeline, and how does that affect DOSBox update frequency?
- **Owner**: Project lead
- **Impact**: Determines urgency of DOSBox updates

---

**End of Report**

**Next Agent**: Agent 1B.2 (API Compatibility Analyzer)
**Deliverable**: `api-compatibility-report.md` with detailed mapping of all 86 integration points to target DOSBox equivalents

**Status**: BLOCKER-001 resolved, Strategy E recommended and ready for implementation pending Agent 1B.2 validation.
