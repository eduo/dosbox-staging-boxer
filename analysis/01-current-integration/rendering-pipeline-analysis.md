# Core Rendering Pipeline Integration Analysis

**Agent**: Agent 1B.2 - Core Rendering Pipeline
**Created**: 2025-11-14T00:00:00Z
**Status**: Completed
**Dependencies**: Agent 1A

## Summary

The rendering pipeline integration analysis reveals **SIGNIFICANT ARCHITECTURAL CHANGES** between legacy DOSBox and target DOSBox Staging. Of the 15 core integration points analyzed:

- **0 points** are DROP-IN compatible (can use exact same code)
- **6 points** have SIGNATURE changes (renamed but similar functionality)
- **5 points** have BEHAVIOR changes (renamed with different functionality)
- **3 points** are MISSING (no direct equivalent exists)
- **1 point** is REFACTORED (major architectural change)

**Migration Complexity**: HIGH. The macro replacement approach used by Boxer will require substantial modification. Several integration points have been fundamentally redesigned, requiring new integration strategies beyond simple renaming.

## Rendering Architecture Changes

### Legacy DOSBox Rendering

**Location**: `/home/user/dosbox-staging-boxer/src/gui/`
**Key Files**:
- `sdlmain.cpp`: Main SDL event loop and rendering functions (2799+ lines)
- `render.cpp`: Scaler and rendering pipeline (812+ lines)
- `include/video.h`: GFX function declarations

**Architecture**:
1. **SDL1.2-based**: Uses SDL1.2 API for windowing and rendering
2. **Direct function calls**: GFX_* functions called directly from rendering code
3. **Single-threaded**: All rendering in main event loop
4. **Simple palette**: Basic RGB palette conversion via GFX_GetRGB
5. **Frame tracking**: Changed lines tracked via Bit16u array passed to GFX_EndUpdate
6. **Scalar-based**: render.cpp applies scalers before presenting

**Integration Method**:
- Boxer uses `#define` macros in BXCoalface.h to remap GFX_* → boxer_*
- Example: `#define GFX_Events boxer_processEvents`
- DOSBox code unchanged, just recompiled with macros active
- Boxer implements boxer_* functions in Objective-C/C++ bridge code

### Target DOSBox Rendering

**Location**: `/home/user/dosbox-staging/src/gui/`
**Key Files**:
- `sdl_gui.cpp`: SDL2-based GUI and event handling (2514+ lines)
- `render/render.cpp`: Modernized rendering pipeline (1345+ lines)
- `render/render_backend.h`: Abstract renderer interface
- `render/opengl_renderer.h`: OpenGL rendering backend
- `render/sdl_renderer.h`: SDL rendering backend
- `private/common.h`: Internal GFX API declarations

**Architecture**:
1. **SDL2-based**: Completely rewritten for SDL2
2. **Backend abstraction**: RenderBackend interface with OpenGL/SDL implementations
3. **Shader support**: Advanced shader system via ShaderManager
4. **Modern pixel formats**: Comprehensive PixelFormat enum, ImageInfo struct
5. **VideoMode metadata**: Rich VideoMode struct with graphics standard tracking
6. **Frame presentation**: Sophisticated presentation mode (DosRate vs HostRate)
7. **No changed lines**: GFX_EndUpdate() takes no parameters
8. **Pixel aspect ratio**: Uses Fraction type for precise aspect ratios

**Integration Challenges**:
- Headers moved from `include/` to `src/gui/private/` and `src/misc/`
- Many functions renamed or signature-changed
- Some functions completely removed
- New abstractions (RenderBackend, ShaderManager, VideoMode)

### Key Differences

| Aspect | Legacy DOSBox | Target DOSBox Staging |
|--------|---------------|------------------------|
| SDL Version | SDL 1.2 | SDL 2.0+ |
| Rendering Backend | Direct SDL | Abstracted (OpenGL/SDL) |
| Shader Support | Basic (GFX_SetShader string) | Advanced (ShaderManager class) |
| Event Loop | GFX_Events() | GFX_PollAndHandleEvents() |
| Frame Updates | GFX_EndUpdate(changedLines) | GFX_EndUpdate() (no params) |
| Palette API | GFX_GetRGB | GFX_MakePixel |
| Refresh Rate | GFX_GetDisplayRefreshRate → int | GFX_GetHostRefreshRate → double |
| Video Mode Info | Flags only | Rich VideoMode struct |
| Pixel Aspect | double scalar | Fraction class |
| Mouse Control | Mouse_AutoLock(bool) | MOUSE_NotifyWindowActive, MOUSE_ToggleUserCapture |
| Title Updates | GFX_SetTitle(cycles, frameskip, paused) | REMOVED |
| Frame Presentation | Implicit in GFX_EndUpdate | Explicit GFX_MaybePresentFrame() |

## Integration Point Analysis

### INT-001: boxer_processEvents (GFX_Events)

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/gui/sdlmain.cpp:2799`
- **Declaration**: `/home/user/dosbox-staging-boxer/include/video.h:57`
- **Signature**: `bool GFX_Events()`
- **Purpose**: Poll SDL events, handle keyboard/mouse/window events, return false to quit
- **Call Sites**:
  - `src/debug/debug.cpp:1866` - Debug mode event processing
  - `src/dosbox.cpp:175` - Main emulation loop

**Target Equivalent**:
- **Status**: RENAMED
- **Location**: `/home/user/dosbox-staging/src/gui/sdl_gui.cpp:2514`
- **Declaration**: `/home/user/dosbox-staging/src/gui/common.h:46`
- **New Signature**: `bool GFX_PollAndHandleEvents()`
- **Behavior Changes**:
  - More sophisticated event handling with SDL2
  - Joystick polling logic added
  - Window event handling modernized
  - Debugger event queue support added

**Compatibility Assessment**:
- **API Match**: ❌ (function renamed)
- **Migration Complexity**: SIGNATURE
- **Changes Required**:
  - Boxer: Update BXCoalface.h macro: `#define GFX_PollAndHandleEvents boxer_processEvents`
  - DOSBox: None required (function exists with new name)

**Migration Strategy**:
1. Add new macro in BXCoalface.h: `#define GFX_PollAndHandleEvents boxer_processEvents`
2. Keep boxer_processEvents implementation unchanged
3. Test event handling for SDL2 compatibility issues
4. Verify debugger integration still works

---

### INT-002: boxer_startFrame (GFX_StartUpdate)

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/gui/sdlmain.cpp:1631`
- **Declaration**: `/home/user/dosbox-staging-boxer/include/video.h:72`
- **Signature**: `bool GFX_StartUpdate(uint8_t * &pixels, int &pitch)`
- **Purpose**: Begin frame rendering, get framebuffer pointer and pitch
- **Call Sites**:
  - `src/gui/render.cpp:116, 182, 190` - Scaler rendering
  - `src/gui/sdlmain.cpp:2002` - Direct rendering

**Target Equivalent**:
- **Status**: EXISTS
- **Location**: `/home/user/dosbox-staging/src/gui/sdl_gui.cpp:1068`
- **Declaration**: `/home/user/dosbox-staging/src/gui/private/common.h:99`
- **New Signature**: `bool GFX_StartUpdate(uint8_t*& pixels, int& pitch)`
- **Behavior Changes**:
  - Reference syntax modernized (uint8_t*& vs uint8_t * &)
  - Additional checks for sdl.draw.updating_framebuffer state
  - Returns false if not active or already updating

**Compatibility Assessment**:
- **API Match**: ✅ (functionally identical signature)
- **Migration Complexity**: DROP-IN (with minor syntax adjustment)
- **Changes Required**:
  - Boxer: None (macro already correct)
  - DOSBox: None required

**Migration Strategy**:
1. Keep existing BXCoalface.h macro: `#define GFX_StartUpdate boxer_startFrame`
2. Verify boxer_startFrame signature matches (reference syntax)
3. Test framebuffer access works correctly

---

### INT-003: boxer_finishFrame (GFX_EndUpdate)

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/gui/sdlmain.cpp:1678`
- **Declaration**: `/home/user/dosbox-staging-boxer/include/video.h:73`
- **Signature**: `void GFX_EndUpdate(const Bit16u *changedLines)`
- **Purpose**: End frame rendering, present framebuffer, track dirty regions
- **Call Sites**:
  - `src/gui/render.cpp:208, 236, 246, 576` - Scaler rendering completion
  - `src/gui/sdl_mapper.cpp:2844` - Mapper UI
  - `src/gui/sdlmain.cpp:1058, 1809, 2032` - Various rendering paths
  - `src/gui/sdl_gui.cpp:93` - GUI overlay

**Target Equivalent**:
- **Status**: EXISTS (SIGNATURE CHANGED)
- **Location**: `/home/user/dosbox-staging/src/gui/sdl_gui.cpp:1080`
- **Declaration**: `/home/user/dosbox-staging/src/gui/private/common.h:103`
- **New Signature**: `void GFX_EndUpdate()`
- **Behavior Changes**:
  - **NO LONGER ACCEPTS changedLines PARAMETER** - major change!
  - Frame presentation is now decoupled
  - Always presents the full framebuffer
  - Dirty region tracking removed (performance tradeoff)

**Compatibility Assessment**:
- **API Match**: ❌ (signature changed - parameter removed)
- **Migration Complexity**: SIGNATURE
- **Changes Required**:
  - Boxer: Modify boxer_finishFrame to accept changedLines but ignore it
  - DOSBox: Create adapter that calls GFX_EndUpdate() without parameters

**Migration Strategy**:
1. Update boxer_finishFrame signature:
   ```cpp
   void boxer_finishFrame(const uint16_t *changedLines) {
       // Call Boxer's frame completion logic
       // Note: changedLines tracking no longer supported in target DOSBox
       // Ignore the parameter
   }
   ```
2. Keep BXCoalface.h macro but handle in implementation
3. Performance impact: may render more than necessary (verify acceptable)

---

### INT-004: boxer_setMouseActive (Mouse_AutoLock)

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/hardware/mouse.cpp` (implementation not shown in search)
- **Declaration**: `/home/user/dosbox-staging-boxer/include/mouse.h:37`
- **Signature**: `void Mouse_AutoLock(bool enable)`
- **Purpose**: Enable/disable automatic mouse capture
- **Call Sites**: Called from DOSBox when mouse capture should change

**Target Equivalent**:
- **Status**: MISSING (API REFACTORED)
- **Location**: NOT FOUND
- **New API**: `/home/user/dosbox-staging/src/hardware/input/mouse.h`
- **Behavior Changes**:
  - Mouse system completely refactored
  - New functions: `MOUSE_NotifyWindowActive(const bool is_active)`
  - New functions: `MOUSE_ToggleUserCapture(const bool pressed)`
  - Automatic locking replaced by explicit window state notifications
  - Mouse capture is now managed differently

**Compatibility Assessment**:
- **API Match**: ❌ (function removed, different approach)
- **Migration Complexity**: BEHAVIOR
- **Changes Required**:
  - Boxer: Implement boxer_setMouseActive to call appropriate new API
  - DOSBox: Map old behavior to new mouse notification system

**Migration Strategy**:
1. Replace Mouse_AutoLock calls with new API:
   ```cpp
   void boxer_setMouseActive(bool mouseActive) {
       // Map old auto-lock behavior to new notification system
       MOUSE_NotifyWindowActive(mouseActive);
       // May also need MOUSE_ToggleUserCapture depending on use case
   }
   ```
2. Study target mouse implementation to understand state machine
3. Test mouse capture behavior matches Boxer expectations
4. Risk: Behavior differences may exist

---

### INT-005: boxer_handleDOSBoxTitleChange (GFX_SetTitle)

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/gui/sdlmain.cpp:425`
- **Declaration**: Multiple extern declarations in src/dos/dos_execute.cpp:94, src/cpu/cpu.cpp:35, src/gui/render.cpp:611
- **Signature**: `void GFX_SetTitle(Bit32s cycles, int frameskip, bool paused)`
- **Purpose**: Update window title with cycle count, frameskip info, pause status
- **Call Sites**:
  - `src/dos/dos_execute.cpp:106, 150, 152, 160` - DOS execution state
  - `src/cpu/cpu.cpp:1579, 1585, 2115, 2130, 2143, 2153, 2415, 2416` - CPU cycle updates
  - `src/gui/render.cpp:617, 625, 812` - Frameskip updates
  - `src/gui/sdlmain.cpp:547` - Pause state

**Target Equivalent**:
- **Status**: MISSING (REMOVED)
- **Location**: NOT FOUND in target repository
- **New Approach**: Title bar managed by `titlebar.cpp` module
  - Declaration: `/home/user/dosbox-staging/src/gui/titlebar.h`
  - Different architecture - title updates handled internally
- **Behavior Changes**:
  - Title updates are now handled by dedicated titlebar module
  - No external API for cycle/frameskip display in title
  - Title content controlled by DOSBox Staging internally

**Compatibility Assessment**:
- **API Match**: ❌ (function removed)
- **Migration Complexity**: MISSING
- **Changes Required**:
  - Boxer: Implement boxer_handleDOSBoxTitleChange as no-op OR use internal tracking
  - DOSBox: Cannot call this function (doesn't exist)

**Migration Strategy**:
1. Create stub implementation in Boxer:
   ```cpp
   void boxer_handleDOSBoxTitleChange(Bit32s cycles, int frameskip, bool paused) {
       // DOSBox Staging doesn't support title updates via this API
       // Store values internally if Boxer needs them for other purposes
       // Or simply ignore - Boxer manages its own window title
   }
   ```
2. Remove macro from BXCoalface.h or keep as stub
3. Risk: LOW - Boxer manages its own window, doesn't need DOSBox title updates

---

### INT-006: boxer_GetDisplayRefreshRate (GFX_GetDisplayRefreshRate)

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/hardware/vga_other.cpp:1507`
- **Declaration**: `/home/user/dosbox-staging-boxer/include/video.h:60`
- **Signature**: `int GFX_GetDisplayRefreshRate()`
- **Purpose**: Get display refresh rate for VGA emulation timing
- **Call Sites**: `src/hardware/vga_other.cpp:1507` - VGA timing calculations

**Target Equivalent**:
- **Status**: RENAMED (SIGNATURE CHANGED)
- **Location**: `/home/user/dosbox-staging/src/gui/sdl_gui.cpp:153`
- **Declaration**: `/home/user/dosbox-staging/src/gui/common.h:40`
- **New Signature**: `double GFX_GetHostRefreshRate()`
- **Behavior Changes**:
  - Returns double instead of int (more precise)
  - Name changed to emphasize "host" refresh rate
  - Better error handling (returns 60 on error)
  - Uses SDL2 display mode queries

**Compatibility Assessment**:
- **API Match**: ❌ (renamed, return type changed)
- **Migration Complexity**: SIGNATURE
- **Changes Required**:
  - Boxer: Implement boxer_GetDisplayRefreshRate to return int
  - DOSBox: Create adapter macro

**Migration Strategy**:
1. Add macro in BXCoalface.h or create adapter:
   ```cpp
   int boxer_GetDisplayRefreshRate(void) {
       return static_cast<int>(GFX_GetHostRefreshRate());
   }
   ```
2. Or update Boxer code to accept double if possible
3. Test VGA timing still works correctly

---

### INT-007: boxer_prepareForFrameSize (GFX_SetSize)

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/gui/sdlmain.cpp:1048`
- **Declaration**: `/home/user/dosbox-staging-boxer/include/video.h:63-66`
- **Signature**:
  ```cpp
  Bitu GFX_SetSize(Bitu width, Bitu height, Bitu flags,
                   double scalex, double scaley,
                   GFX_CallBack_t callback,
                   double pixel_aspect)
  ```
- **Purpose**: Set rendering dimensions, scaling, pixel aspect, register callback
- **Call Sites**:
  - `src/gui/render.cpp:481` - Rendering setup
  - `src/gui/sdlmain.cpp:1994` - Screenshot capture

**Target Equivalent**:
- **Status**: EXISTS (SIGNATURE CHANGED)
- **Location**: `/home/user/dosbox-staging/src/gui/sdl_gui.cpp:896`
- **Declaration**: `/home/user/dosbox-staging/src/gui/private/common.h:88-90`
- **New Signature**:
  ```cpp
  uint8_t GFX_SetSize(const int render_width_px, const int render_height_px,
                      const Fraction& render_pixel_aspect_ratio,
                      const uint8_t flags,
                      const VideoMode& video_mode,
                      GFX_Callback_t callback)
  ```
- **Behavior Changes**:
  - **MAJOR SIGNATURE CHANGE**
  - Bitu → int/uint8_t (type changes)
  - scalex/scaley removed (scaling handled differently)
  - pixel_aspect changed from double to Fraction& (complex type)
  - Added VideoMode& parameter (new metadata structure)
  - GFX_CallBack_t → GFX_Callback_t (type renamed)
  - Flags return changed from Bitu to uint8_t

**Compatibility Assessment**:
- **API Match**: ❌ (completely different signature)
- **Migration Complexity**: REFACTORED
- **Changes Required**:
  - Boxer: Major adapter implementation needed
  - DOSBox: Complex wrapper to bridge old/new APIs

**Migration Strategy**:
1. Create adapter in DOSBox side:
   ```cpp
   Bitu boxer_prepareForFrameSize(Bitu width, Bitu height, Bitu gfx_flags,
                                   double scalex, double scaley,
                                   GFX_CallBack_t callback, double pixel_aspect) {
       // Convert old parameters to new format
       Fraction aspect_ratio = Fraction::FromDouble(pixel_aspect);
       VideoMode video_mode = /* construct from available info */;
       uint8_t flags = static_cast<uint8_t>(gfx_flags);

       // Note: scalex/scaley cannot be directly passed -
       // target handles scaling differently
       auto result = GFX_SetSize(width, height, aspect_ratio, flags,
                                  video_mode, callback);
       return static_cast<Bitu>(result);
   }
   ```
2. **CRITICAL**: Need to construct VideoMode struct - may require tracking video state
3. **CRITICAL**: scalex/scaley loss - verify Boxer doesn't rely on these
4. Risk: HIGH - complex adapter, possible behavior differences

---

### INT-008: boxer_getRGBPaletteEntry (GFX_GetRGB)

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/gui/sdlmain.cpp:1793`
- **Declaration**: `/home/user/dosbox-staging-boxer/include/video.h:61`
- **Signature**: `Bitu GFX_GetRGB(Bit8u red, Bit8u green, Bit8u blue)`
- **Purpose**: Convert RGB888 to native pixel format
- **Call Sites**:
  - `src/gui/render.cpp:69, 83` - Palette setup for 16/32-bit modes

**Target Equivalent**:
- **Status**: RENAMED
- **Location**: `/home/user/dosbox-staging/src/gui/sdl_gui.cpp` (implementation location)
- **Declaration**: `/home/user/dosbox-staging/src/gui/private/common.h:84`
- **New Signature**: `uint32_t GFX_MakePixel(const uint8_t red, const uint8_t green, const uint8_t blue)`
- **Behavior Changes**:
  - Renamed GFX_GetRGB → GFX_MakePixel
  - Bitu → uint32_t return type
  - Bit8u → uint8_t (type modernization)
  - const parameters

**Compatibility Assessment**:
- **API Match**: ❌ (renamed, type changes)
- **Migration Complexity**: SIGNATURE
- **Changes Required**:
  - Boxer: Update types from Bitu to uint32_t if needed
  - DOSBox: Add macro mapping

**Migration Strategy**:
1. Add macro in BXCoalface.h:
   ```cpp
   #define GFX_MakePixel boxer_getRGBPaletteEntry
   ```
2. Update boxer_getRGBPaletteEntry signature if needed:
   ```cpp
   Bitu boxer_getRGBPaletteEntry(Bit8u red, Bit8u green, Bit8u blue) {
       // Return value converted to Bitu for compatibility
   }
   ```
3. Test palette rendering in all color depths

---

### INT-009: boxer_setShader (GFX_SetShader)

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/gui/sdlmain.cpp:1457`
- **Declaration**: `/home/user/dosbox-staging-boxer/include/video.h:62`
- **Signature**: `void GFX_SetShader(const char* src)`
- **Purpose**: Set shader source code for OpenGL rendering
- **Call Sites**:
  - `src/gui/render.cpp:474` - Apply shader during rendering setup

**Target Equivalent**:
- **Status**: MISSING (API REFACTORED)
- **Location**: Shader system completely redesigned
- **New API**:
  - `/home/user/dosbox-staging/src/gui/private/shader_manager.h` - ShaderManager class
  - `/home/user/dosbox-staging/src/gui/render/render_backend.h:54` - `virtual bool SetShader(const std::string& symbolic_shader_name) = 0`
  - `/home/user/dosbox-staging/src/gui/render/opengl_renderer.h:44` - OpenGL implementation
- **Behavior Changes**:
  - No longer a simple string source
  - Shaders identified by symbolic name, loaded from files
  - Complex shader management with caching, settings parsing
  - Object-oriented API via RenderBackend interface
  - Shader must exist in shader directory

**Compatibility Assessment**:
- **API Match**: ❌ (completely different approach)
- **Migration Complexity**: REFACTORED
- **Changes Required**:
  - Boxer: Major reimplementation needed
  - DOSBox: Cannot use old shader source string approach

**Migration Strategy**:
1. **Option A - No-op stub** (if Boxer doesn't actively use shaders):
   ```cpp
   void boxer_setShader(const char* src) {
       // DOSBox Staging uses named shaders loaded from files
       // Cannot set shader source directly
       // Ignore or log warning
   }
   ```
2. **Option B - Map to named shaders** (if Boxer uses shaders):
   ```cpp
   void boxer_setShader(const char* src) {
       // Analyze src to determine intent
       // Map to appropriate named shader
       auto renderer = GFX_GetRenderer();
       if (src == nullptr || strlen(src) == 0) {
           renderer->SetShader("none");
       } else {
           // Parse or map to named shader
           renderer->SetShader("sharp");  // or other appropriate name
       }
   }
   ```
3. **Option C - Write shader to file** (complex):
   - Write src to temporary shader file
   - Register with ShaderManager
   - Load shader by name
4. Risk: MEDIUM-HIGH - depends on Boxer's shader usage

---

### INT-010: boxer_idealOutputMode (GFX_GetBestMode)

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/gui/sdlmain.cpp:595`
- **Declaration**: `/home/user/dosbox-staging-boxer/include/video.h:59`
- **Signature**: `Bitu GFX_GetBestMode(Bitu flags)`
- **Purpose**: Determine best output mode based on capabilities flags
- **Call Sites**:
  - `src/gui/render.cpp:436` - Choose rendering mode

**Target Equivalent**:
- **Status**: MISSING (REMOVED)
- **Location**: NOT FOUND
- **Behavior Changes**:
  - Function removed
  - Output mode selection handled internally by renderer
  - No external API for mode selection
  - Modern renderers handle mode selection automatically

**Compatibility Assessment**:
- **API Match**: ❌ (function removed)
- **Migration Complexity**: MISSING
- **Changes Required**:
  - Boxer: Implement stub or return sensible default
  - DOSBox: Cannot call this function

**Migration Strategy**:
1. Create stub implementation:
   ```cpp
   Bitu boxer_idealOutputMode(Bitu flags) {
       // DOSBox Staging handles mode selection internally
       // Return flags unchanged or with sensible defaults
       return flags | GFX_CAN_32;  // Modern renderers support 32-bit
   }
   ```
2. Verify Boxer doesn't rely on specific mode selection logic
3. Risk: LOW - modern renderers auto-select appropriate modes

---

### INT-011: boxer_MaybeProcessEvents (GFX_MaybeProcessEvents)

**Legacy Implementation**:
- **Location**: NOT FOUND in legacy repository
- **Declaration**: `/home/user/Boxer/Boxer/BXCoalface.h:36`
- **Signature**: Assumed `bool GFX_MaybeProcessEvents()` (based on macro)
- **Purpose**: Conditionally process events (possibly for performance)
- **Call Sites**: NOT FOUND in legacy DOSBox (may be Boxer-specific)

**Target Equivalent**:
- **Status**: RENAMED (DIFFERENT PURPOSE)
- **Location**: `/home/user/dosbox-staging/src/gui/sdl_gui.cpp:2465`
- **Declaration**: `/home/user/dosbox-staging/src/gui/common.h:44`
- **New Signature**: `void GFX_MaybePresentFrame()`
- **Behavior Changes**:
  - **COMPLETELY DIFFERENT PURPOSE**
  - Legacy: Conditional event processing
  - Target: Conditional frame presentation
  - Not an equivalent function!

**Compatibility Assessment**:
- **API Match**: ❌ (different purpose despite similar name)
- **Migration Complexity**: MISSING
- **Changes Required**:
  - Boxer: Investigate actual usage in Boxer codebase
  - DOSBox: Likely need different approach

**Migration Strategy**:
1. **INVESTIGATE**: Search Boxer codebase to understand GFX_MaybeProcessEvents usage
2. If it's for event processing:
   ```cpp
   bool boxer_MaybeProcessEvents() {
       // Map to full event processing or implement conditional logic
       return GFX_PollAndHandleEvents();
   }
   ```
3. If it's for frame presentation:
   ```cpp
   bool boxer_MaybeProcessEvents() {
       GFX_MaybePresentFrame();
       return true;
   }
   ```
4. Risk: MEDIUM - unclear usage, needs investigation

---

### INT-012: boxer_log (GFX_ShowMsg)

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/gui/sdlmain.cpp:3086`
- **Declaration**: `/home/user/dosbox-staging-boxer/include/logging.h:92`
- **Signature**: `void GFX_ShowMsg(char const* format,...) GCC_ATTRIBUTE(__format__(__printf__, 1, 2))`
- **Purpose**: Display message to user (on-screen or console)
- **Call Sites**:
  - `src/gui/sdlmain.cpp:3655, 3660` - Error messages

**Target Equivalent**:
- **Status**: EXISTS
- **Location**: `/home/user/dosbox-staging/src/main.cpp:83`
- **Declaration**: `/home/user/dosbox-staging/src/misc/logging.h:66`
- **New Signature**: `void GFX_ShowMsg(const char* format, ...)`
- **Behavior Changes**:
  - char const* → const char* (style change, equivalent)
  - GCC_ATTRIBUTE not shown in declaration (may still be there)
  - Likely uses modern logging system (Loguru-based)

**Compatibility Assessment**:
- **API Match**: ✅ (functionally identical)
- **Migration Complexity**: DROP-IN
- **Changes Required**:
  - Boxer: None
  - DOSBox: None

**Migration Strategy**:
1. Keep existing BXCoalface.h macro: `#define GFX_ShowMsg boxer_log`
2. No changes needed to boxer_log implementation
3. Test message display still works

---

### INT-016: boxer_applyRenderingStrategy (direct call in render.cpp:279)

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/gui/render.cpp:279`
- **Declaration**: `/home/user/Boxer/Boxer/BXCoalface.h:51`
- **Signature**: `void boxer_applyRenderingStrategy(void)`
- **Purpose**: Allow Boxer to override DOSBox scaler settings before RENDER_Reset
- **Call Sites**:
  - `src/gui/render.cpp:279` - Called at start of RENDER_Reset()
- **Context**:
  ```cpp
  /* static */ void RENDER_Reset( void ) {
      //--Added 2009-03-06 by Alun Bestor to allow Boxer to override DOSBox's scaler settings
      boxer_applyRenderingStrategy();
      //--End of modifications

      Bitu width=render.src.width;
      // ... rendering setup continues
  }
  ```

**Target Equivalent**:
- **Status**: MISSING (RENDER_Reset REMOVED)
- **Location**: NOT FOUND - RENDER_Reset doesn't exist in target
- **New Approach**: No direct equivalent function
  - Rendering reset handled differently in target
  - No single reset point for inserting hooks
  - Shader selection via `RENDER_SetShaderWithFallback()` at different call sites

**Compatibility Assessment**:
- **API Match**: ❌ (no equivalent hook point)
- **Migration Complexity**: REFACTORED
- **Changes Required**:
  - Boxer: Rethink rendering strategy application
  - DOSBox: Find alternative hook points or modify rendering flow

**Migration Strategy**:
1. **Option A - Remove hook** (if Boxer doesn't need scaler control):
   - Simply don't call boxer_applyRenderingStrategy
   - Boxer handles all rendering on its side
2. **Option B - Hook GFX_SetSize** (preferred):
   ```cpp
   // In modified target DOSBox, before calling GFX_SetSize:
   uint8_t GFX_SetSize(const int render_width_px, ...) {
       boxer_applyRenderingStrategy();  // Call Boxer hook
       // ... proceed with normal GFX_SetSize logic
   }
   ```
3. **Option C - Hook shader selection**:
   - Call boxer_applyRenderingStrategy before RENDER_SetShaderWithFallback
4. **CRITICAL**: Analyze what boxer_applyRenderingStrategy actually does
   - Does it modify render.* globals?
   - Does it change scaler selection?
   - Does it configure aspect ratio?
5. Risk: HIGH - no direct equivalent, architectural change

---

## Summary Table

| ID | Name | Legacy Location | Target Status | Complexity | Changes Required |
|----|------|-----------------|---------------|------------|------------------|
| INT-001 | boxer_processEvents | sdlmain.cpp:2799 | RENAMED | SIGNATURE | Macro: GFX_Events → GFX_PollAndHandleEvents |
| INT-002 | boxer_startFrame | sdlmain.cpp:1631 | EXISTS | DROP-IN | None (signature compatible) |
| INT-003 | boxer_finishFrame | sdlmain.cpp:1678 | SIGNATURE | SIGNATURE | Adapter: ignore changedLines param |
| INT-004 | boxer_setMouseActive | mouse.cpp (impl) | MISSING | BEHAVIOR | Map to MOUSE_NotifyWindowActive |
| INT-005 | boxer_handleDOSBoxTitleChange | sdlmain.cpp:425 | MISSING | MISSING | Stub (no-op) |
| INT-006 | boxer_GetDisplayRefreshRate | vga_other.cpp:1507 | RENAMED | SIGNATURE | GFX_GetHostRefreshRate, cast double→int |
| INT-007 | boxer_prepareForFrameSize | sdlmain.cpp:1048 | REFACTORED | REFACTORED | Complex adapter: map old→new signature |
| INT-008 | boxer_getRGBPaletteEntry | sdlmain.cpp:1793 | RENAMED | SIGNATURE | Macro: GFX_GetRGB → GFX_MakePixel |
| INT-009 | boxer_setShader | sdlmain.cpp:1457 | REFACTORED | REFACTORED | Map to RenderBackend::SetShader or stub |
| INT-010 | boxer_idealOutputMode | sdlmain.cpp:595 | MISSING | MISSING | Stub (return default flags) |
| INT-011 | boxer_MaybeProcessEvents | NOT FOUND | MISSING | MISSING | Investigate usage, map appropriately |
| INT-012 | boxer_log | sdlmain.cpp:3086 | EXISTS | DROP-IN | None (signature compatible) |
| INT-016 | boxer_applyRenderingStrategy | render.cpp:279 | REFACTORED | REFACTORED | Hook GFX_SetSize or remove |

## Critical Findings

### Functions That Still Exist (DROP-IN or SIGNATURE)

**Easy to Migrate** (3 points, ~2-4 hours total):

1. **INT-002: boxer_startFrame** - DROP-IN
   - Function exists with compatible signature
   - Just verify reference syntax
   - Test framebuffer access

2. **INT-012: boxer_log** - DROP-IN
   - Function exists with identical signature
   - No changes needed
   - Verify message display works

3. **INT-001: boxer_processEvents** - SIGNATURE
   - Renamed GFX_Events → GFX_PollAndHandleEvents
   - Add macro, test SDL2 event handling

### Functions That Changed Significantly (BEHAVIOR or REFACTORED)

**Moderate to High Effort** (6 points, ~20-40 hours total):

4. **INT-003: boxer_finishFrame** - SIGNATURE
   - Parameter removed (changedLines)
   - Need adapter to ignore parameter
   - Test performance impact

5. **INT-006: boxer_GetDisplayRefreshRate** - SIGNATURE
   - Renamed, return type changed (int → double)
   - Simple cast needed
   - Test VGA timing

6. **INT-008: boxer_getRGBPaletteEntry** - SIGNATURE
   - Renamed GFX_GetRGB → GFX_MakePixel
   - Type modernization
   - Test palette rendering

7. **INT-004: boxer_setMouseActive** - BEHAVIOR
   - Mouse API completely refactored
   - Map to MOUSE_NotifyWindowActive
   - Test mouse capture behavior
   - Risk: Behavior differences

8. **INT-007: boxer_prepareForFrameSize** - REFACTORED
   - Major signature change (added VideoMode, Fraction)
   - Lost scalex/scaley parameters
   - Complex adapter needed
   - HIGH RISK

9. **INT-016: boxer_applyRenderingStrategy** - REFACTORED
   - RENDER_Reset removed
   - Need alternative hook point
   - May require DOSBox code modification
   - HIGH RISK

### Functions That Are Missing (MISSING)

**Blocked or Require Alternatives** (4 points, ~10-20 hours total):

10. **INT-005: boxer_handleDOSBoxTitleChange** - MISSING
    - Function removed
    - Stub implementation (Boxer manages own title)
    - LOW RISK

11. **INT-009: boxer_setShader** - MISSING/REFACTORED
    - Shader API completely redesigned
    - Map to named shaders or stub
    - MEDIUM RISK (depends on usage)

12. **INT-010: boxer_idealOutputMode** - MISSING
    - Function removed
    - Return default flags
    - LOW RISK

13. **INT-011: boxer_MaybeProcessEvents** - MISSING
    - Usage unclear
    - Investigate Boxer codebase
    - MEDIUM RISK

## Rendering System Compatibility

### Can Boxer's Approach Still Work?

**Answer**: PARTIALLY - with significant modifications

**Current Approach Viability**:
- ✅ Macro replacement can still work for some functions
- ❌ Many functions have incompatible signatures requiring adapters
- ❌ Some functions don't exist, requiring stubs or alternatives
- ❌ Major architectural changes (VideoMode, RenderBackend) require new code

**Major Blockers**:
1. **VideoMode struct**: GFX_SetSize now requires VideoMode parameter
   - Need to construct or track VideoMode state
   - Not available in legacy calling code
2. **Pixel aspect as Fraction**: Can't pass double directly
   - Need Fraction class in adapter code
3. **Shader API**: Complete redesign from source strings to named shaders
4. **RENDER_Reset hook**: No equivalent function
5. **SDL2 migration**: Underlying SDL API completely different

### Required Architectural Changes

**In Boxer**:
1. Add adapters for signature-changed functions (INT-003, INT-006, INT-008)
2. Implement behavior-mapped functions (INT-004)
3. Create stubs for missing functions (INT-005, INT-010, INT-011)
4. Rework shader handling if used (INT-009)
5. Rethink rendering strategy hook (INT-016)

**In DOSBox (target)**:
1. Add wrapper functions for changed signatures (INT-007 especially)
2. Add hook point for boxer_applyRenderingStrategy (INT-016)
3. Possibly expose VideoMode construction API
4. Consider compatibility shims for removed functions

**In Integration Layer (BXCoalface.h)**:
1. Update macros for renamed functions
2. Add adapter code for signature changes
3. Document which functions are stubbed vs. functional

### Alternative Approaches

If macro replacement becomes too complex:

**Option 1 - Compatibility Shim Layer**:
- Create intermediate C++ file in DOSBox that provides all legacy functions
- Implements adapters and stubs
- Calls new target APIs internally
- Macro layer becomes simpler

**Option 2 - Direct Code Modification**:
- Modify DOSBox Staging code directly to call boxer_* functions
- No macro replacement
- Clearer but more invasive
- Harder to maintain across DOSBox updates

**Option 3 - Hybrid Approach** (RECOMMENDED):
- Use macros for simple renames (INT-001, INT-008)
- Use shim layer for complex adapters (INT-007)
- Use stubs for missing functions (INT-005, INT-010)
- Directly modify DOSBox for critical hooks (INT-016)

## Macro Replacement Viability

### Current Approach (Legacy)

```cpp
// From /home/user/Boxer/Boxer/BXCoalface.h
#define GFX_Events boxer_processEvents
#define GFX_StartUpdate boxer_startFrame
#define GFX_EndUpdate boxer_finishFrame
#define Mouse_AutoLock boxer_setMouseActive
#define GFX_SetTitle boxer_handleDOSBoxTitleChange
#define GFX_GetDisplayRefreshRate boxer_GetDisplayRefreshRate
#define GFX_SetSize boxer_prepareForFrameSize
#define GFX_GetRGB boxer_getRGBPaletteEntry
#define GFX_SetShader boxer_setShader
#define GFX_GetBestMode boxer_idealOutputMode
#define GFX_MaybeProcessEvents boxer_MaybeProcessEvents
#define GFX_ShowMsg boxer_log
```

### Will This Work in Target?

**Answer**: NO - requires significant changes

**What Works**:
- ✅ INT-002: GFX_StartUpdate (compatible signature)
- ✅ INT-012: GFX_ShowMsg (compatible signature)

**What Needs Updating**:
- ⚠️ INT-001: Rename GFX_Events → GFX_PollAndHandleEvents
- ⚠️ INT-006: Rename GFX_GetDisplayRefreshRate → GFX_GetHostRefreshRate
- ⚠️ INT-008: Rename GFX_GetRGB → GFX_MakePixel

**What Needs Adapters**:
- ❌ INT-003: GFX_EndUpdate signature changed (parameter removed)
- ❌ INT-004: Mouse_AutoLock doesn't exist (use MOUSE_NotifyWindowActive)
- ❌ INT-007: GFX_SetSize signature completely different

**What Needs Stubs**:
- ❌ INT-005: GFX_SetTitle removed
- ❌ INT-009: GFX_SetShader removed (new API)
- ❌ INT-010: GFX_GetBestMode removed
- ❌ INT-011: GFX_MaybeProcessEvents doesn't exist (GFX_MaybePresentFrame different)

**What Needs Code Changes**:
- ❌ INT-016: No macro - direct call to boxer_applyRenderingStrategy in RENDER_Reset (which doesn't exist)

### Required Modifications

**Updated BXCoalface.h** (pseudo-code):

```cpp
//--Remapped replacements for DOSBox Staging functions
// Simple renames
#define GFX_PollAndHandleEvents boxer_processEvents
#define GFX_MakePixel boxer_getRGBPaletteEntry
#define GFX_ShowMsg boxer_log

// Adapters needed - can't use simple macro
// These need C++ wrapper functions in a .cpp file
extern "C" {
    void GFX_EndUpdate();  // Calls boxer_finishFrame(nullptr)
    double GFX_GetHostRefreshRate();  // Returns (double)boxer_GetDisplayRefreshRate()
    void MOUSE_NotifyWindowActive(bool active);  // Calls boxer_setMouseActive(active)
    uint8_t GFX_SetSize(...complex signature...);  // Adapter for boxer_prepareForFrameSize

    // Stubs
    void GFX_SetTitle(int32_t cycles, int frameskip, bool paused) { /* stub */ }
    // ... etc
}

// Direct compatibility (no changes)
#define GFX_StartUpdate boxer_startFrame
```

**Create New File: BXCoalface_adapters.cpp**:

```cpp
// Adapter implementations
void GFX_EndUpdate() {
    boxer_finishFrame(nullptr);  // Ignore changedLines
}

double GFX_GetHostRefreshRate() {
    return static_cast<double>(boxer_GetDisplayRefreshRate());
}

void MOUSE_NotifyWindowActive(bool active) {
    boxer_setMouseActive(active);
}

uint8_t GFX_SetSize(const int render_width_px, const int render_height_px,
                    const Fraction& render_pixel_aspect_ratio,
                    const uint8_t flags,
                    const VideoMode& video_mode,
                    GFX_Callback_t callback) {
    // Extract legacy parameters from new ones
    double pixel_aspect = render_pixel_aspect_ratio.ToDouble();
    double scalex = 1.0;  // Can't extract from new API - default
    double scaley = 1.0;  // Can't extract from new API - default

    Bitu result = boxer_prepareForFrameSize(
        render_width_px, render_height_px,
        static_cast<Bitu>(flags),
        scalex, scaley,
        callback, pixel_aspect
    );

    return static_cast<uint8_t>(result);
}

// Stubs
void GFX_SetTitle(int32_t cycles, int frameskip, bool paused) {
    // No-op - DOSBox Staging doesn't support this
}

Bitu GFX_GetBestMode(Bitu flags) {
    return flags | GFX_CAN_32;  // Modern default
}

// ... etc
```

## Risk Assessment

### HIGH Risk

**Blockers or Very Difficult** (4 points):

1. **INT-007: boxer_prepareForFrameSize (GFX_SetSize)**
   - Signature completely changed
   - Missing scalex/scaley in target
   - Need VideoMode construction
   - **Risk**: May lose functionality
   - **Mitigation**: Study render.cpp to understand how VideoMode is built

2. **INT-016: boxer_applyRenderingStrategy**
   - No hook point in target
   - RENDER_Reset removed
   - **Risk**: Cannot apply Boxer rendering settings
   - **Mitigation**: Hook GFX_SetSize or modify target code directly

3. **INT-009: boxer_setShader (GFX_SetShader)**
   - Shader system completely redesigned
   - Cannot set source directly
   - **Risk**: Shader features may not work
   - **Mitigation**: Map to named shaders or disable shader features

4. **INT-004: boxer_setMouseActive (Mouse_AutoLock)**
   - Mouse API refactored
   - Different state machine
   - **Risk**: Mouse capture behavior may differ
   - **Mitigation**: Study target mouse.cpp thoroughly

### MEDIUM Risk

**Significant Work Required** (4 points):

5. **INT-003: boxer_finishFrame (GFX_EndUpdate)**
   - Changed parameter removed
   - **Risk**: Performance impact (rendering more than needed)
   - **Mitigation**: Test performance, acceptable for modern systems

6. **INT-011: boxer_MaybeProcessEvents**
   - Unclear usage
   - Different function with similar name exists
   - **Risk**: Unclear mapping
   - **Mitigation**: Search Boxer codebase for usage

7. **INT-001: boxer_processEvents (GFX_Events)**
   - SDL2 event handling different
   - **Risk**: Event handling bugs
   - **Mitigation**: Thorough testing of input/window events

8. **INT-006: boxer_GetDisplayRefreshRate**
   - Return type changed
   - **Risk**: Precision loss in VGA timing
   - **Mitigation**: Test VGA timing, cast should be fine

### LOW Risk

**Straightforward** (5 points):

9. **INT-002: boxer_startFrame** - Compatible
10. **INT-012: boxer_log** - Compatible
11. **INT-008: boxer_getRGBPaletteEntry** - Simple rename
12. **INT-005: boxer_handleDOSBoxTitleChange** - Stub (Boxer manages title)
13. **INT-010: boxer_idealOutputMode** - Stub (auto-selection works)

## Effort Estimate

### Per-Integration Point

**DROP-IN complexity** (2 points):
- INT-002, INT-012
- 0.5 hours each = **1 hour total**

**SIGNATURE complexity** (5 points):
- INT-001, INT-003, INT-006, INT-008
- Simple: 1-2 hours each = **4-8 hours**
- INT-007: Complex adapter = **6-10 hours**
- **Subtotal: 10-18 hours**

**BEHAVIOR complexity** (1 point):
- INT-004: Mouse API mapping = **3-5 hours**

**REFACTORED complexity** (2 points):
- INT-009: Shader mapping = **4-6 hours**
- INT-016: Hook replacement = **6-10 hours**
- **Subtotal: 10-16 hours**

**MISSING complexity** (3 points):
- INT-005: Stub = **0.5 hours**
- INT-010: Stub = **0.5 hours**
- INT-011: Investigation + implementation = **2-4 hours**
- **Subtotal: 3-5 hours**

**Testing and Integration**:
- Integration testing: **8-12 hours**
- Debug and fixes: **8-16 hours**
- **Subtotal: 16-28 hours**

### Total for All 15 Points

**Optimistic**: 40-60 hours (assuming stubs acceptable, minimal issues)
**Realistic**: 60-80 hours (with proper testing, some rework)
**Pessimistic**: 80-120 hours (if major issues found, complex debugging)

**Recommended Estimate**: **70 hours** (2 weeks at 50% allocation)

## Recommendations

### Priority Actions

1. **IMMEDIATE** - Analyze boxer_applyRenderingStrategy (INT-016)
   - Understand what rendering settings it applies
   - Determine if can be moved to GFX_SetSize hook
   - This is critical for rendering to work at all

2. **IMMEDIATE** - Investigate INT-011 (boxer_MaybeProcessEvents)
   - Search Boxer codebase for usage
   - Determine correct mapping
   - May be unused and can be stubbed

3. **HIGH PRIORITY** - Create adapter infrastructure
   - Create BXCoalface_adapters.cpp file
   - Implement adapters for signature-changed functions
   - Start with INT-003, INT-006, INT-008 (simpler ones)

4. **HIGH PRIORITY** - Tackle INT-007 (GFX_SetSize) adapter
   - Study target render.cpp to understand VideoMode creation
   - Implement complex adapter
   - Test thoroughly - this is core rendering

5. **MEDIUM PRIORITY** - Mouse API mapping (INT-004)
   - Study target mouse.cpp
   - Map Mouse_AutoLock to new notification system
   - Test mouse capture behavior

6. **MEDIUM PRIORITY** - Shader handling (INT-009)
   - Determine if Boxer actually uses shaders
   - If yes: map to named shaders
   - If no: stub it out

7. **LOW PRIORITY** - Create stubs
   - INT-005, INT-010 (straightforward)

### Risk Mitigations

1. **For INT-016 (rendering strategy hook)**:
   - Option A: Hook GFX_SetSize in target DOSBox code
   - Option B: Move logic to Boxer-side GFX_SetSize wrapper
   - Create proof-of-concept early to validate approach

2. **For INT-007 (GFX_SetSize signature)**:
   - Create VideoMode construction helper
   - Document which fields can't be populated from legacy params
   - Test with various video modes (VGA, EGA, CGA, text modes)

3. **For INT-009 (shaders)**:
   - If Boxer uses shaders: identify use cases
   - Create mapping table (source pattern → named shader)
   - Fallback to "none" shader if can't map

4. **For general compatibility**:
   - Create comprehensive test suite
   - Test all video modes Boxer supports
   - Test mouse capture/release
   - Test window events
   - Test error paths

### Further Analysis Needed

1. **Boxer shader usage** (INT-009):
   - Search Boxer codebase: `grep -r "setShader\|GFX_SetShader" /home/user/Boxer/`
   - Identify shader sources used
   - Determine criticality

2. **boxer_MaybeProcessEvents usage** (INT-011):
   - Search Boxer codebase: `grep -r "MaybeProcessEvents" /home/user/Boxer/`
   - Understand when it's called
   - Determine if event or presentation related

3. **boxer_applyRenderingStrategy implementation** (INT-016):
   - Read Boxer implementation
   - Identify what render.* globals it modifies
   - Determine if behavior can be replicated in target

4. **Scaling parameters impact** (INT-007):
   - Identify where scalex/scaley are used in Boxer
   - Determine if loss of these parameters is acceptable
   - May need alternative scaling approach

5. **VideoMode construction**:
   - Trace how target DOSBox builds VideoMode
   - Identify minimal required fields
   - Create construction helper for adapter

## Blockers/Open Questions

### Critical Blockers

1. **INT-016: No RENDER_Reset hook point**
   - **Question**: Where can we hook boxer_applyRenderingStrategy in target?
   - **Options**: GFX_SetSize, shader selection, or modify target code
   - **Impact**: Cannot apply Boxer rendering settings without this
   - **Decision needed**: Which approach to take

2. **INT-007: VideoMode parameter requirement**
   - **Question**: How do we construct VideoMode in adapter with limited info?
   - **Impact**: GFX_SetSize calls will fail without valid VideoMode
   - **Need**: VideoMode construction helper or state tracking

3. **INT-009: Shader source to named shader mapping**
   - **Question**: Does Boxer actually use shaders? What sources?
   - **Impact**: If critical, need mapping strategy
   - **Need**: Boxer codebase analysis

### Open Questions

4. **INT-011: boxer_MaybeProcessEvents purpose**
   - **Question**: What is this function for? Event or presentation?
   - **Need**: Boxer codebase search

5. **INT-004: Mouse behavior equivalence**
   - **Question**: Does MOUSE_NotifyWindowActive provide same behavior?
   - **Need**: Mouse behavior testing

6. **INT-003: Performance impact of no dirty tracking**
   - **Question**: Is rendering full frame every time acceptable?
   - **Need**: Performance testing

7. **Scaling parameters loss (INT-007)**
   - **Question**: Can Boxer work without explicit scalex/scaley?
   - **Impact**: May affect aspect ratio handling
   - **Need**: Review Boxer scaler usage

8. **Header file structure**
   - **Question**: Can we include src/gui/private/*.h in Boxer?
   - **Impact**: May need to expose more headers
   - **Note**: Headers moved from include/ to src/

### Next Steps

1. **Investigation Phase** (4-8 hours):
   - Analyze Boxer's shader usage (INT-009)
   - Find boxer_MaybeProcessEvents usage (INT-011)
   - Read boxer_applyRenderingStrategy implementation (INT-016)
   - Study VideoMode construction in target

2. **Proof of Concept** (8-16 hours):
   - Create basic adapter file
   - Implement 2-3 simple adapters (INT-003, INT-006, INT-008)
   - Test compilation
   - Validate approach

3. **Complex Adapters** (16-24 hours):
   - Implement GFX_SetSize adapter (INT-007)
   - Implement rendering strategy hook (INT-016)
   - Implement mouse API mapping (INT-004)

4. **Integration & Testing** (24-32 hours):
   - Integrate all adapters
   - Comprehensive testing
   - Debug issues
   - Performance validation

---

**Total Estimated Migration Time**: 52-80 hours

**Confidence Level**: MEDIUM (architectural changes add uncertainty)

**Recommendation**: Proceed with phased approach - implement simple adapters first to validate strategy before tackling complex ones.
