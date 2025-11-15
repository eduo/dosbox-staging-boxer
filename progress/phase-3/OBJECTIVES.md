# Phase 3: Rendering & Display - Objectives

**Phase**: 3
**Duration**: Weeks 5-6
**Estimated Hours**: 60-80
**Status**: NOT STARTED

---

## Primary Goal
Video output working with Metal backend, all video modes functional.

---

## Objectives

### 1. SDL2 to Metal Bridge
Connect DOSBox's SDL2 rendering to Boxer's Metal backend.

**Success Criteria**:
- Frame buffer data accessible to Boxer
- Metal textures created at correct resolution
- Pixel formats properly handled

### 2. Frame Rendering Pipeline
Implement startFrame/finishFrame hooks for frame capture.

**Success Criteria**:
- Every frame captured
- Data uploaded to GPU
- 60+ FPS performance

### 3. Video Mode Support
Handle all DOSBox video modes (text, CGA, EGA, VGA, SVGA).

**Success Criteria**:
- Mode switching works without crash
- All resolutions render correctly
- Palette changes reflected

### 4. Event Integration
Integrate Boxer's macOS event loop with DOSBox events.

**Success Criteria**:
- Events properly forwarded
- No input lag
- Responsive UI

---

## Deliverables

1. **Code**:
   - Frame hooks in render.cpp
   - BXDOSBoxRenderer Metal integration
   - Palette handling
   - Mode switching logic

2. **Tests**:
   - All video mode tests
   - Frame rate measurement
   - Color accuracy tests

3. **Documentation**:
   - Rendering architecture
   - Performance metrics

---

## Dependencies

**Prerequisites**:
- Phase 2 complete
- Lifecycle control working
- Metal rendering infrastructure

**Blocking Decisions**:
- None

---

## Phase Exit Criteria

- [ ] DOS programs display correctly
- [ ] All video modes work
- [ ] Colors accurate
- [ ] 60+ FPS maintained
- [ ] Mode switching smooth
- [ ] Human review approved

**Ready for Phase 4 when all criteria met.**
