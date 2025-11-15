# Phase 3: Rendering & Display - Objectives

**Phase Start Date**: 2025-11-15
**Estimated Duration**: 60-80 hours (2 weeks)
**Status**: IN PROGRESS

---

## Phase Goal

**Enable Boxer to display DOSBox video output using Metal backend.**

By the end of Phase 3:
- DOS programs display correctly on screen
- All video modes functional (text, CGA, EGA, VGA, SVGA)
- Frame rendering pipeline connected to Metal
- Palette and color handling working
- Mode switching without crashes
- 60+ FPS performance maintained

**This is the first phase where VISIBLE OUTPUT appears.**

---

## Critical Integration Points (10 hooks)

From `analysis/01-current-integration/integration-overview.md`:

1. **INT-001**: boxer_processEvents (GFX_Events) - Event loop integration
2. **INT-002**: boxer_startFrame (GFX_StartUpdate) - Frame rendering start
3. **INT-003**: boxer_finishFrame (GFX_EndUpdate) - Frame rendering complete
4. **INT-007**: boxer_prepareForFrameSize (GFX_SetSize) - Mode/resolution changes
5. **INT-008**: boxer_getRGBPaletteEntry (GFX_GetRGB) - Palette queries
6. **INT-010**: boxer_idealOutputMode (GFX_GetBestMode) - Output mode selection

Plus 4 additional rendering-related hooks to be identified during analysis.

---

## Phase 3 Tasks

### TASK 3-1: SDL2 to Metal Bridge Analysis
**Estimated**: 10-12 hours | **Criticality**: CORE | **Risk**: HIGH

**Objective**: Understand how target DOSBox uses SDL2 for rendering and design bridge to Boxer's Metal backend.

**Deliverables**:
- Analysis document mapping SDL2 to Metal
- Identify all hook points for frame data
- Design texture upload strategy
- Prototype renderer skeleton

**Success Criteria**:
- [ ] Complete understanding of SDL2 rendering path
- [ ] Metal bridge strategy documented
- [ ] No blocking architectural issues

---

### TASK 3-2: Frame Buffer Hooks
**Estimated**: 14-18 hours | **Criticality**: CORE | **Risk**: HIGH

**Objective**: Implement startFrame and finishFrame hooks to capture frame data from DOSBox.

---

### TASK 3-3: Metal Texture Upload
**Estimated**: 12-16 hours | **Criticality**: CORE | **Risk**: MEDIUM

**Objective**: Upload DOSBox frame buffer to Metal texture for display.

---

### TASK 3-4: Video Mode Switching
**Estimated**: 10-14 hours | **Criticality**: MAJOR | **Risk**: MEDIUM

**Objective**: Handle DOSBox video mode changes (resolution, color depth, text vs. graphics).

---

### TASK 3-5: Palette Handling
**Estimated**: 8-12 hours | **Criticality**: MAJOR | **Risk**: LOW

**Objective**: Handle color palette changes for indexed color modes.

---

### TASK 3-6: Event Processing Integration
**Estimated**: 6-8 hours | **Criticality**: MAJOR | **Risk**: LOW

**Objective**: Integrate Boxer's event loop with DOSBox's event processing.

---

## Success Criteria for Phase 3

### Rendering Pipeline
- [ ] Frame buffer hooks functional
- [ ] Metal texture upload working
- [ ] Display shows DOSBox output
- [ ] Colors correct (no swapping)

### Video Modes
- [ ] Text mode renders properly
- [ ] CGA modes work (320x200, 640x200)
- [ ] EGA modes work (640x350)
- [ ] VGA modes work (640x480, 320x200)
- [ ] SVGA modes work (800x600, 1024x768+)
- [ ] Mode switching smooth and crash-free

### Performance
- [ ] 60+ FPS maintained
- [ ] Frame upload <2ms
- [ ] No dropped frames

### Integration
- [ ] Event processing integrated
- [ ] Window management working
- [ ] Aspect ratio correct

---

## Validation Gates

### Gate 0: Pre-Phase Checklist ✅
- [x] Phase 3 objectives documented (this file)
- [x] Success criteria defined
- [x] Estimated hours reviewed (60-80h)
- [x] Dependencies satisfied (Phases 1 & 2 complete)
- [x] Required analysis documents identified

### Gate 1: Static Analysis
After EACH file modification:
- [ ] All new code compiles
- [ ] No missing includes
- [ ] All `#ifdef BOXER_INTEGRATED` blocks properly closed

### Gate 2: Consistency Check
After completing all tasks:
- [ ] All 10 rendering hooks implemented
- [ ] All video modes tested
- [ ] Performance requirements met

### Gate 3: Human Review
Before advancing to Phase 4:
- [ ] Human reviews all task reports
- [ ] Rendering quality acceptable
- [ ] Performance acceptable

---

## Risks and Mitigations

### HIGH Risks

1. **SDL2/Metal Impedance Mismatch**
   - Risk: DOSBox SDL2 assumptions incompatible with Metal
   - Mitigation: TASK 3-1 analysis identifies issues early

2. **Frame Buffer Performance**
   - Risk: Texture upload too slow for 60 FPS
   - Mitigation: Profile early, optimize upload path

### MEDIUM Risks

1. **Mode Switching Complexity**
   - Risk: Many edge cases in mode switching
   - Mitigation: Comprehensive test suite

---

**Phase Status**: IN PROGRESS ⏳
**Next Task**: TASK 3-1 (SDL2/Metal Analysis)
**Blocking Issues**: None
