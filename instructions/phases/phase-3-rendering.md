# Phase 3: Rendering & Display - Agent Tasks

**Phase Duration**: Weeks 5-6
**Total Estimated Hours**: 60-80 hours
**Goal**: Video output working with Metal backend

**Prerequisites**: Phase 2 complete (lifecycle control working)

---

## IMPORTANT: Repository Structure

**Root**: `/home/user/dosbox-staging-boxer/`

**Two SEPARATE Repositories**:
1. DOSBox Staging (`src/dosbox-staging/`) - Branch: `dosbox-boxer-upgrade-dosboxside`
2. Boxer (`src/boxer/`) - Branch: `boxer-dosbox-upgrade-boxerside`

**Phase 3 modifies**: Both DOSBox Staging (rendering hooks) and Boxer (Metal backend)

---

## PHASE 3 OVERVIEW

By the end of Phase 3, you will have:
- DOS programs display correctly on screen
- All video modes functional (text, CGA, EGA, VGA, SVGA)
- Frame rendering pipeline connected to Metal
- Palette and color handling working
- Mode switching without crashes
- 60+ FPS performance

**This phase produces VISIBLE OUTPUT for the first time.**

---

## CRITICAL INTEGRATION POINTS

From integration-overview.md:
- **INT-001**: boxer_processEvents (macro → GFX_Events)
- **INT-002**: boxer_startFrame (macro → GFX_StartUpdate)
- **INT-003**: boxer_finishFrame (macro → GFX_EndUpdate)
- **INT-007**: boxer_prepareForFrameSize (macro → GFX_SetSize)
- **INT-008**: boxer_getRGBPaletteEntry (macro → GFX_GetRGB)
- **INT-010**: boxer_idealOutputMode (macro → GFX_GetBestMode)

---

## TASK 3-1: SDL2 to Metal Bridge Analysis

### Context
- **Phase**: 3
- **Estimated Hours**: 10-12 hours
- **Criticality**: CORE
- **Risk Level**: HIGH

### Objective
Understand how target DOSBox uses SDL2 for rendering and design bridge to Boxer's Metal backend.

### Prerequisites
- [ ] Phase 2 complete
- [ ] Target DOSBox compiles (library build)
- [ ] Understand Metal rendering basics

### Input Documents
1. `src/dosbox-staging/src/gui/sdlmain.cpp`
   - SDL2 window and rendering setup
   - Frame buffer management

2. `src/dosbox-staging/src/gui/render.cpp`
   - Rendering pipeline
   - Frame callback points

3. `src/boxer/Boxer/BXVideoHandler.m`
   - Current Metal rendering implementation
   - Texture management

### Deliverables
1. **Analysis document**: `progress/phase-3/RENDERING_ANALYSIS.md`
   - Map SDL2 calls to Metal equivalents
   - Identify hook points for frame data
   - Design texture upload strategy
   
2. **Prototype**: `src/boxer/Boxer/BXDOSBoxRenderer.mm`
   - Skeleton implementation
   - Frame buffer interface
   
3. **Documentation**: `progress/phase-3/tasks/TASK-3-1.md`

### Key Questions to Answer
1. How does target DOSBox provide frame buffer data?
2. What format is the pixel data (RGB, RGBA, indexed)?
3. How are palette changes communicated?
4. What triggers a frame update?
5. How is vertical sync handled?

### Success Criteria
- [ ] Complete understanding of SDL2 rendering path
- [ ] Metal bridge strategy documented
- [ ] No blocking architectural issues identified

---

## TASK 3-2: Frame Buffer Hooks

### Context
- **Phase**: 3
- **Estimated Hours**: 14-18 hours
- **Criticality**: CORE
- **Risk Level**: HIGH

### Objective
Implement startFrame and finishFrame hooks to capture frame data from DOSBox.

### Prerequisites
- [ ] TASK 3-1 complete (rendering analysis done)
- [ ] Understand target DOSBox frame structure

### Deliverables
1. **Modified**: `src/dosbox-staging/src/gui/sdlmain.cpp` or `render.cpp`
   - Add BOXER_HOOK for frame start
   - Add BOXER_HOOK for frame end
   - Pass frame buffer pointer to Boxer
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement startFrame() - prepare Metal texture
   - Implement finishFrame() - upload pixels to GPU
   
3. **Documentation**: `progress/phase-3/tasks/TASK-3-2.md`

### Implementation Pattern

```cpp
// In DOSBox rendering code
bool GFX_StartUpdate(uint8_t* &pixels, uint16_t &pitch) {
    #ifdef BOXER_INTEGRATED
    // Let Boxer provide frame buffer
    if (!BOXER_HOOK_BOOL(startFrame, &pixels, &pitch)) {
        return false;
    }
    return true;
    #else
    // Standard SDL path
    // ...
    #endif
}

void GFX_EndUpdate(const uint16_t *changedLines) {
    #ifdef BOXER_INTEGRATED
    BOXER_HOOK_VOID(finishFrame, changedLines);
    #else
    // Standard SDL path
    #endif
}
```

### Decision Points - STOP if:

1. **Frame format mismatch**: DOSBox outputs format Metal can't directly use
   - Options: A) Convert in DOSBox, B) Convert in Boxer, C) Use different texture format
   - Report: What formats involved

2. **Buffer ownership unclear**: Who allocates frame buffer memory
   - Options: A) Boxer allocates, B) DOSBox allocates, C) Shared
   - Report: Memory management strategy needed

### Success Criteria
- [ ] Frame hooks called for each frame
- [ ] Pixel data accessible to Boxer
- [ ] No crashes during rendering
- [ ] Correct pitch/stride handling

---

## TASK 3-3: Metal Texture Upload

### Context
- **Phase**: 3
- **Estimated Hours**: 12-16 hours
- **Criticality**: CORE
- **Risk Level**: MEDIUM

### Objective
Upload DOSBox frame buffer to Metal texture for display.

### Prerequisites
- [ ] TASK 3-2 complete (frame hooks working)
- [ ] Frame buffer data accessible

### Deliverables
1. **Modified**: `src/boxer/Boxer/BXDOSBoxRenderer.mm`
   - Create Metal texture matching DOSBox resolution
   - Upload pixel data to texture
   - Handle format conversion if needed
   
2. **Modified**: `src/boxer/Boxer/BXVideoHandler.m`
   - Display Metal texture in view
   - Handle aspect ratio
   
3. **Performance test**: Texture upload timing
   
4. **Documentation**: `progress/phase-3/tasks/TASK-3-3.md`

### Metal Upload Pattern

```objc
// In BXDOSBoxRenderer.mm
- (void)uploadFrameBuffer:(uint8_t*)pixels 
                    width:(uint16_t)width 
                   height:(uint16_t)height 
                    pitch:(uint16_t)pitch {
    // Create texture if needed
    if (!_texture || _width != width || _height != height) {
        MTLTextureDescriptor *desc = [MTLTextureDescriptor 
            texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                         width:width
                                        height:height
                                     mipmapped:NO];
        _texture = [_device newTextureWithDescriptor:desc];
        _width = width;
        _height = height;
    }
    
    // Upload pixels
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    [_texture replaceRegion:region
                mipmapLevel:0
                  withBytes:pixels
                bytesPerRow:pitch];
}
```

### Success Criteria
- [ ] Texture created at correct resolution
- [ ] Pixels uploaded without corruption
- [ ] Format matches (no color swapping)
- [ ] Performance adequate (<2ms per frame)

---

## TASK 3-4: Video Mode Switching

### Context
- **Phase**: 3
- **Estimated Hours**: 10-14 hours
- **Criticality**: MAJOR
- **Risk Level**: MEDIUM

### Objective
Handle DOSBox video mode changes (resolution, color depth, text vs. graphics).

### Prerequisites
- [ ] TASK 3-3 complete (basic rendering works)

### Input Documents
1. `analysis/01-current-integration/integration-overview.md`
   - INT-007: prepareForFrameSize

2. `src/dosbox-staging/src/gui/render.cpp`
   - Mode change logic

### Deliverables
1. **Modified**: `src/dosbox-staging/src/gui/render.cpp`
   - Add BOXER_HOOK for mode change
   - Pass new dimensions and format
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement prepareForFrameSize()
   - Recreate textures at new size
   - Adjust window if needed
   
3. **Test**: Mode switching test
   - Switch through all modes
   - Verify correct rendering
   
4. **Documentation**: `progress/phase-3/tasks/TASK-3-4.md`

### Modes to Support
- Text mode (80x25, 80x50, etc.)
- CGA (320x200, 640x200)
- EGA (640x350)
- VGA (640x480, 320x200)
- SVGA (800x600, 1024x768, etc.)

### Success Criteria
- [ ] Mode changes don't crash
- [ ] Texture resized correctly
- [ ] No visual glitches during switch
- [ ] All standard modes work

---

## TASK 3-5: Palette Handling

### Context
- **Phase**: 3
- **Estimated Hours**: 8-12 hours
- **Criticality**: MAJOR
- **Risk Level**: LOW

### Objective
Handle color palette changes for indexed color modes.

### Prerequisites
- [ ] TASK 3-4 complete (modes switching)

### Input Documents
1. `analysis/01-current-integration/integration-overview.md`
   - INT-008: getRGBPaletteEntry

2. `src/dosbox-staging/src/ints/int10_pal.cpp`
   - Palette management

### Deliverables
1. **Modified**: DOSBox palette code
   - Add BOXER_HOOK for palette change
   - Export palette entries to Boxer
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement getRGBPaletteEntry()
   - Update Metal shader uniforms
   
3. **Documentation**: `progress/phase-3/tasks/TASK-3-5.md`

### Implementation Approaches
1. **Immediate mode**: Convert indexed to RGB in DOSBox before hook
2. **Palette texture**: Pass palette as separate texture, convert in shader
3. **CPU conversion**: Convert all pixels in Boxer before upload

### Success Criteria
- [ ] Colors display correctly in all modes
- [ ] Palette changes reflected immediately
- [ ] No color banding or artifacts
- [ ] Performance maintained

---

## TASK 3-6: Event Processing Integration

### Context
- **Phase**: 3
- **Estimated Hours**: 6-8 hours
- **Criticality**: MAJOR
- **Risk Level**: LOW

### Objective
Integrate Boxer's event loop with DOSBox's event processing.

### Prerequisites
- [ ] TASK 3-5 complete (rendering fully working)

### Deliverables
1. **Modified**: `src/dosbox-staging/src/gui/sdlmain.cpp`
   - Add BOXER_HOOK for event processing
   - Allow Boxer to handle macOS events
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement processEvents()
   - Forward relevant events to DOSBox
   
3. **Documentation**: `progress/phase-3/tasks/TASK-3-6.md`

### Success Criteria
- [ ] Boxer receives all macOS events
- [ ] DOSBox doesn't block event loop
- [ ] Responsive UI (no lag)
- [ ] Events properly forwarded

---

## PHASE 3 COMPLETION CHECKLIST

### Rendering Pipeline ✅
- [ ] Frame buffer hooks functional
- [ ] Metal texture upload working
- [ ] Display shows DOSBox output
- [ ] Colors correct (no swapping)

### Video Modes ✅
- [ ] Text mode renders properly
- [ ] CGA modes work
- [ ] EGA modes work
- [ ] VGA modes work
- [ ] SVGA modes work
- [ ] Mode switching smooth

### Performance ✅
- [ ] 60+ FPS maintained
- [ ] Frame upload <2ms
- [ ] No dropped frames
- [ ] Smooth animation

### Integration ✅
- [ ] Event processing integrated
- [ ] Window management working
- [ ] Full-screen mode functional
- [ ] Aspect ratio correct

**When all boxes checked, Phase 3 is complete. Ready for Phase 4 (Shell).**

---

## ESTIMATED TIME BREAKDOWN

- TASK 3-1: SDL2/Metal Analysis - 10-12 hours
- TASK 3-2: Frame Buffer Hooks - 14-18 hours
- TASK 3-3: Metal Texture Upload - 12-16 hours
- TASK 3-4: Video Mode Switching - 10-14 hours
- TASK 3-5: Palette Handling - 8-12 hours
- TASK 3-6: Event Processing - 6-8 hours

**Total**: 60-80 hours

**Calendar time**: 2 weeks
