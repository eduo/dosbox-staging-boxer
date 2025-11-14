# Graphics Modes Integration Analysis

**Agent**: Agent 1B.6 - Graphics Modes
**Created**: 2025-11-14T15:41:31+00:00
**Status**: Completed
**Dependencies**: Agent 1A

## Summary

All 6 graphics mode integration points (Hercules tinting and CGA composite) remain fully supported in target DOSBox-staging with runtime configurability. The underlying implementation uses modern C++ patterns (enums and knob objects) but exposes equivalent functionality. Migration requires wrapper functions to adapt between Boxer's uint8_t/double interfaces and the target's enum/knob types. User impact is minimal - these user-facing features can be preserved with straightforward adapters.

## Legacy Graphics Modes

### Hercules Tinting

**What it is**: Hercules Graphics Cards were monochrome, but could display in different phosphor colors to simulate the appearance of various period-accurate monitors.

**How it works**: The `herc_pal` variable (values 0-2) controls which color palette is applied:
- 0 = Amber (warm orange/amber tint)
- 1 = Green (classic green phosphor)
- 2 = White (paper-white monochrome)

**Implementation**: When tint mode changes, `Herc_Palette()` updates the DAC color entries to apply the selected tint, then `VGA_DAC_CombineColor(1,7)` refreshes the display.

### CGA Composite Mode

**What it is**: Early CGA cards produced RGBI digital output, but when connected to composite monitors, created color artifacts that games exploited for additional colors and effects.

**Color artifacts**: By manipulating pixel patterns, developers could create up to 16 colors from CGA's 4-color palette through composite video interference patterns.

**Authenticity**: This was a real technique used in 1980s games. Titles like King's Quest, Ultima, and many Sierra games looked dramatically different (and better) on composite vs RGB monitors.

**Implementation**:
- `cga_comp` controls mode (AUTO/ON/OFF)
- `hue` knob adjusts color phase offset (-360 to +360)
- Complex composite signal simulation generates proper NTSC color bleeding
- `update_cga16_color()` recalculates the composite lookup table when settings change

### User-Facing Features

Boxer exposes these settings through its UI to let users customize the authentic retro experience:

**Hercules Tinting** (View menu):
- 3 tint options: Amber, Green, White
- Stored in user defaults as `herculesTintMode`
- Bound to BXDOSWindowController for UI updates

**CGA Composite** (View menu):
- 3 mode options: Auto, On, Off
- Hue adjustment slider (continuous double value)
- Stored in user defaults as `CGACompositeMode`
- Allows users to dial in authentic period colors

## Target Graphics System

### File Structure

Target has reorganized VGA code into subdirectory:
- **Legacy**: `/home/user/dosbox-staging-boxer/src/hardware/vga_other.cpp`
- **Target**: `/home/user/dosbox-staging/src/hardware/video/vga_other.cpp`

The file still exists and contains graphics mode handling code.

### Graphics Mode Support

**CGA**: Fully supported with enhanced composite simulation
- Multiple composite modes (M_CGA2_COMPOSITE, M_CGA4_COMPOSITE, M_CGA16)
- Advanced NTSC composite algorithm (reenigne's implementation)
- Separate handling for old-era vs new-era CGA revisions
- Runtime composite state toggling via hotkeys (F10/F11/F12)

**Hercules**: Fully supported with expanded palettes
- Graphics mode (M_HERC_GFX) and text mode (M_HERC_TEXT)
- 4 palette options (added Paperwhite as 4th option)
- Hotkey cycling (F11)

**EGA/VGA**: Fully supported (not relevant to these integration points)

### Configuration Approach

**Runtime API**: All settings remain runtime-configurable through code:
- `hercules_palette` variable (enum MonochromePalette)
- `cga_comp` variable (enum COMPOSITE_STATE)
- `hue`, `saturation`, `contrast`, `brightness`, `convergence` knobs (knob_t objects)

**Config File**: Settings also exposed in `dosbox.conf` [composite] section:
- `composite = auto|on|off`
- `era = auto|old|new`
- `hue = -360 to 360`
- `saturation = 0 to 360`
- `contrast = 0 to 360`
- `brightness = -100 to 100`
- `convergence = -50 to 50`

**Hybrid approach**: Config provides defaults, runtime API allows dynamic changes.

## Integration Point Analysis

### INT-017: boxer_herculesTintMode

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/hardware/vga_other.cpp:1454`
- **Purpose**: Query current Hercules tint setting
- **Signature**: `Bit8u boxer_herculesTintMode(void)`
- **Return Type**: `Bit8u` (0=Amber, 1=Green, 2=White)
- **Implementation**: `return herc_pal;`

**Target Equivalent**:
- **Status**: REFACTORED (different data structure)
- **Location**: `/home/user/dosbox-staging/src/hardware/video/vga_other.cpp:254`
- **Variable**: `static MonochromePalette hercules_palette = {};`
- **Type**: `enum class MonochromePalette : uint8_t { Amber=0, Green=1, White=2, Paperwhite=3 }`
- **Changes**:
  - Legacy uses `uint8_t herc_pal` (3 values: 0-2)
  - Target uses `MonochromePalette` enum (4 values: 0-3)
  - Target added 4th palette (Paperwhite)

**Compatibility**: REFACTORED (type conversion needed)

**Migration Strategy**:
```cpp
Bit8u boxer_herculesTintMode(void)
{
    extern MonochromePalette hercules_palette;
    // Cast enum to uint8_t - values 0-2 are compatible
    // Note: Target has 4th value (Paperwhite=3), but Boxer only uses 0-2
    return static_cast<Bit8u>(hercules_palette);
}
```

---

### INT-018: boxer_setHerculesTintMode

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/hardware/vga_other.cpp:1459`
- **Purpose**: Set Hercules tint mode
- **Signature**: `void boxer_setHerculesTintMode(Bit8u mode)`
- **Parameters**: `mode` - 0=Amber, 1=Green, 2=White (modulo 3)
- **Implementation**:
  ```cpp
  if (herc_pal != mode) {
      herc_pal = mode % 3;
      if (machine == MCH_HERC) {
          Herc_Palette();
          VGA_DAC_CombineColor(1,7);
      }
  }
  ```

**Target Equivalent**:
- **Status**: REFACTORED (use public API)
- **Location**: `/home/user/dosbox-staging/src/hardware/video/vga_other.cpp:1177`
- **Function**: `void VGA_SetMonochromePalette(const enum MonochromePalette _palette)`
- **Changes**:
  - Legacy sets variable directly and calls internal functions
  - Target has public API function `VGA_SetMonochromePalette()`
  - Target function checks machine type internally
  - Target calls `VGA_SetHerculesPalette()` internally

**Compatibility**: REFACTORED (use wrapper to call public API)

**Migration Strategy**:
```cpp
void boxer_setHerculesTintMode(Bit8u mode)
{
    // Validate input (Boxer sends 0-2, target supports 0-3)
    if (mode > 2) {
        mode = mode % 3;  // Legacy behavior: wrap to 0-2
    }

    // Cast to enum and call target's public API
    MonochromePalette palette = static_cast<MonochromePalette>(mode);
    VGA_SetMonochromePalette(palette);
}
```

**Note**: Target's `VGA_SetMonochromePalette()` already handles the machine check and palette update, so we don't need to duplicate that logic.

---

### INT-019: boxer_CGACompositeHueOffset

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/hardware/vga_other.cpp:1472`
- **Purpose**: Query CGA composite hue offset
- **Signature**: `double boxer_CGACompositeHueOffset(void)`
- **Return Type**: `double` (range -360.0 to 360.0)
- **Implementation**: `return hue.get();`

**Target Equivalent**:
- **Status**: EXISTS (identical implementation)
- **Location**: `/home/user/dosbox-staging/src/hardware/video/vga_other.cpp:239`
- **Variable**: `static knob_t hue(0, -360, 360);`
- **Type**: `knob_t` class with `int get()` method
- **Changes**:
  - Legacy: `return hue.get();` (knob returns int, implicitly cast to double)
  - Target: Identical `knob_t` class with same signature
  - Both use same default (0) and range (-360 to 360)

**Compatibility**: DROP-IN (identical code, just cast int→double)

**Migration Strategy**:
```cpp
double boxer_CGACompositeHueOffset(void)
{
    extern knob_t hue;
    return static_cast<double>(hue.get());
}
```

**Note**: This is a trivial wrapper - the knob_t class is identical in both versions.

---

### INT-020: boxer_setCGACompositeHueOffset

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/hardware/vga_other.cpp:1477`
- **Purpose**: Set CGA composite hue offset
- **Signature**: `void boxer_setCGACompositeHueOffset(double offset)`
- **Parameters**: `offset` - hue adjustment in degrees (-360.0 to 360.0)
- **Implementation**:
  ```cpp
  if (offset != hue.get()) {
      hue.set(offset);
      if (machine == MCH_CGA) {
          update_cga16_color();
      }
  }
  ```

**Target Equivalent**:
- **Status**: EXISTS (identical implementation)
- **Location**: `/home/user/dosbox-staging/src/hardware/video/vga_other.cpp:239`
- **Variable**: `static knob_t hue(0, -360, 360);`
- **Method**: `void set(int new_val)` (accepts int, wraps to range)
- **Changes**:
  - Legacy checks machine type and calls `update_cga16_color()`
  - Target has same pattern in hotkey handlers
  - Need to replicate machine check and update logic

**Compatibility**: REFACTORED (need wrapper with machine check)

**Migration Strategy**:
```cpp
void boxer_setCGACompositeHueOffset(double offset)
{
    extern knob_t hue;

    // Cast double to int for knob_t::set()
    int hue_value = static_cast<int>(std::round(offset));

    if (hue_value != hue.get()) {
        hue.set(hue_value);

        // Update composite colors if in CGA mode
        if (is_machine_cga() || is_machine_pcjr() || is_machine_tandy()) {
            if (is_machine_pcjr()) {
                update_cga16_color_pcjr();
            } else {
                update_cga16_color();
            }
        }
    }
}
```

**Note**: Target has separate update functions for PCjr vs CGA/Tandy, need to call the right one.

---

### INT-021: boxer_CGAComponentMode

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/hardware/vga_other.cpp:1490`
- **Purpose**: Query CGA composite mode (Auto/On/Off)
- **Signature**: `Bit8u boxer_CGAComponentMode(void)`
- **Return Type**: `Bit8u` (0=Auto, 1=On, 2=Off)
- **Implementation**: `return (Bit8u)cga_comp;`

**Target Equivalent**:
- **Status**: EXISTS (identical enum)
- **Location**: `/home/user/dosbox-staging/src/hardware/video/vga_other.cpp:251`
- **Variable**: `static COMPOSITE_STATE cga_comp = COMPOSITE_STATE::AUTO;`
- **Type**: `enum class COMPOSITE_STATE : uint8_t { AUTO=0, ON=1, OFF=2 }`
- **Changes**:
  - Legacy uses same enum values
  - Target uses identical enum definition
  - No changes needed

**Compatibility**: DROP-IN (identical enum)

**Migration Strategy**:
```cpp
Bit8u boxer_CGAComponentMode(void)
{
    extern COMPOSITE_STATE cga_comp;
    return static_cast<Bit8u>(cga_comp);
}
```

**Note**: This is trivial - both versions use the exact same enum with same values.

---

### INT-022: boxer_setCGAComponentMode

**Legacy Implementation**:
- **Location**: `/home/user/dosbox-staging-boxer/src/hardware/vga_other.cpp:1495`
- **Purpose**: Set CGA composite mode (Auto/On/Off)
- **Signature**: `void boxer_setCGAComponentMode(Bit8u newCGA)`
- **Parameters**: `newCGA` - 0=Auto, 1=On, 2=Off (validated, >2 → Auto)
- **Implementation**:
  ```cpp
  cga_comp = COMPOSITE_STATE(newCGA);
  if ((Bit8u)cga_comp > 2) cga_comp = COMPOSITE_STATE::AUTO;
  if (vga.tandy.mode_control & 0x2) {
      write_cga(0x3d8, vga.tandy.mode_control, io_width_t::byte);
  }
  ```

**Target Equivalent**:
- **Status**: EXISTS (use public function)
- **Location**: `/home/user/dosbox-staging/src/hardware/video/vga_other.cpp:808`
- **Function**: `static void apply_composite_state()`
- **Changes**:
  - Legacy sets variable and triggers mode switch manually
  - Target has `apply_composite_state()` helper that handles mode switching
  - Target checks `is_machine_pcjr()` vs CGA/Tandy for different logic paths

**Compatibility**: REFACTORED (use wrapper with apply function)

**Migration Strategy**:
```cpp
void boxer_setCGAComponentMode(Bit8u newCGA)
{
    extern COMPOSITE_STATE cga_comp;

    // Validate input
    if (newCGA > 2) {
        newCGA = 0;  // Invalid values become AUTO
    }

    // Set the mode
    cga_comp = static_cast<COMPOSITE_STATE>(newCGA);

    // Apply the composite state (triggers mode switch)
    apply_composite_state();
}
```

**Note**: Target's `apply_composite_state()` handles all the machine-specific logic for switching between composite and RGB modes. This is cleaner than the legacy manual write_cga() call.

---

## Summary Table

| ID | Name | Mode | Target Status | Complexity | User Impact |
|----|------|------|---------------|------------|-------------|
| INT-017 | herculesTintMode | Hercules | REFACTORED | LOW | None - values 0-2 compatible |
| INT-018 | setHerculesTintMode | Hercules | REFACTORED | LOW | None - use public API |
| INT-019 | CGACompositeHueOffset | CGA | DROP-IN | TRIVIAL | None - int→double cast |
| INT-020 | setCGACompositeHueOffset | CGA | REFACTORED | MEDIUM | None - need machine check |
| INT-021 | CGAComponentMode | CGA | DROP-IN | TRIVIAL | None - identical enum |
| INT-022 | setCGAComponentMode | CGA | REFACTORED | LOW | None - use helper function |

## Graphics Mode Compatibility

### Hercules Support

**Status**: FULLY SUPPORTED (enhanced)

**Tinting**: Runtime configurable via code and hotkeys
- Legacy: 3 palettes (Amber, Green, White)
- Target: 4 palettes (added Paperwhite)
- Boxer only needs 3, so fully compatible

**API Changes**:
- Legacy: Direct variable access (`herc_pal`)
- Target: Public API function (`VGA_SetMonochromePalette()`)
- Target enum is more type-safe

**Migration**: Straightforward wrapper functions
- Query: Cast enum to uint8_t
- Setter: Cast uint8_t to enum, call public API

### CGA Composite Support

**Status**: FULLY SUPPORTED (enhanced)

**Hue Control**: Runtime configurable
- Legacy: knob_t object with int storage
- Target: Identical knob_t implementation
- Migration: Trivial int↔double conversion

**Component Mode**: Runtime configurable
- Legacy: COMPOSITE_STATE enum
- Target: Identical enum definition
- Migration: Direct cast, use apply_composite_state()

**Enhanced Features in Target**:
- Separate old-era vs new-era CGA simulation
- Enhanced NTSC composite algorithm
- Additional knobs: saturation, contrast, brightness, convergence
- Hotkey system for runtime adjustment (F10/F11/F12)
- Boxer doesn't expose these extra features yet, but could in future

**Migration**: Wrapper functions needed
- Hue setter needs machine type check and proper update call
- Mode setter should use `apply_composite_state()` helper

## User Impact

### If Runtime Configuration Lost

If these APIs were removed, Boxer would lose:

1. **Hercules Tinting Menu** - Users couldn't switch between Amber/Green/White
2. **CGA Composite Toggle** - Users couldn't enable/disable composite mode at runtime
3. **CGA Hue Adjustment** - Users couldn't dial in period-accurate colors

**User-visible features affected**:
- View → Hercules Tinting submenu (3 items)
- View → CGA Composite Mode submenu (3 items)
- CGA Hue slider (wherever exposed in UI)

**Impact severity**: MEDIUM - These are nice-to-have retro authenticity features, not critical functionality. Games will run fine with default settings, but enthusiasts appreciate the customization.

### Alternative Approaches

Since runtime API **does exist** in target, no alternatives needed. However, if it didn't:

**Plan B - Config file manipulation**:
1. Write dosbox.conf [composite] section before launching game
2. Restart emulator when user changes settings
3. Awkward UX, but would work

**Plan C - Reimplementation**:
1. Add Boxer-specific graphics mode code to target
2. Maintain own hercules_palette and cga_comp variables
3. Hook into VGA rendering pipeline
4. High maintenance burden

**Chosen approach**: Use target's existing runtime API with thin wrapper functions (Plan A).

## Migration Complexity

**Total Effort**: 2-4 hours

**Breakdown**:
- INT-017: 15 min (trivial enum cast)
- INT-018: 30 min (call public API)
- INT-019: 10 min (trivial wrapper)
- INT-020: 60 min (machine check logic, update calls)
- INT-021: 10 min (trivial enum cast)
- INT-022: 45 min (apply_composite_state integration)
- Testing: 60 min (verify all modes work correctly)

**Scenarios**:

✅ **If runtime API exists**: LOW effort (2-4 hours)
- Create 6 wrapper functions in BXCoalface.mm
- Declare extern variables (hercules_palette, cga_comp, hue)
- Test Hercules tinting on MCH_HERC
- Test CGA composite on MCH_CGA
- Test hue adjustment slider

❌ **If config-only**: MEDIUM effort (8-16 hours) - NOT NEEDED
- Parse/write dosbox.conf composite section
- Restart emulator on changes (bad UX)
- Would require significant configuration plumbing

❌ **If removed**: HIGH effort (40-80 hours) - NOT NEEDED
- Reimplement Hercules palette system
- Reimplement CGA composite simulation
- Fork and maintain graphics code
- Ongoing merge burden

## Risks

### LOW Risks

1. **Enum value mismatch** (Hercules)
   - **Issue**: Target has 4 palettes (0-3), Boxer uses 3 (0-2)
   - **Mitigation**: Validate input in setter, modulo 3 if >2
   - **Probability**: Low - Boxer's UI only sends valid values

2. **Double precision loss** (CGA hue)
   - **Issue**: Boxer uses double, target uses int
   - **Mitigation**: Round when converting double→int
   - **Impact**: Negligible - hue adjustments are coarse (degree increments)
   - **Probability**: Very low - unlikely to matter in practice

3. **Machine type detection timing**
   - **Issue**: Setters check machine type - what if called before machine initialized?
   - **Mitigation**: Target's own code does same checks, so safe pattern
   - **Probability**: Very low - Boxer only calls after emulator running

4. **Update function naming** (CGA)
   - **Issue**: Need to call `update_cga16_color()` vs `update_cga16_color_pcjr()`
   - **Mitigation**: Use machine type check like target's hotkey handlers
   - **Probability**: Low - pattern exists in target code

### MEDIUM Risks

None identified. All APIs exist with compatible interfaces.

### HIGH Risks

None identified. This is a straightforward migration.

## Recommendations

### Priority 1: Implement Wrapper Functions (2 hours)

Create `/home/user/dosbox-staging/src/hardware/video/boxer_graphics_modes.cpp`:

```cpp
// Boxer graphics mode integration - wrapper functions for legacy API

#include "vga.h"

// External references to target's variables
extern MonochromePalette hercules_palette;
extern COMPOSITE_STATE cga_comp;
extern knob_t hue;

// Declare target's update functions
void update_cga16_color();
void update_cga16_color_pcjr();
void apply_composite_state();

// INT-017: Query Hercules tint mode
Bit8u boxer_herculesTintMode(void)
{
    return static_cast<Bit8u>(hercules_palette);
}

// INT-018: Set Hercules tint mode
void boxer_setHerculesTintMode(Bit8u mode)
{
    if (mode > 2) mode = mode % 3;  // Boxer uses 0-2 only
    VGA_SetMonochromePalette(static_cast<MonochromePalette>(mode));
}

// INT-019: Query CGA composite hue offset
double boxer_CGACompositeHueOffset(void)
{
    return static_cast<double>(hue.get());
}

// INT-020: Set CGA composite hue offset
void boxer_setCGACompositeHueOffset(double offset)
{
    int hue_value = static_cast<int>(std::round(offset));
    if (hue_value != hue.get()) {
        hue.set(hue_value);
        if (is_machine_cga() || is_machine_pcjr() || is_machine_tandy()) {
            if (is_machine_pcjr()) {
                update_cga16_color_pcjr();
            } else {
                update_cga16_color();
            }
        }
    }
}

// INT-021: Query CGA composite mode
Bit8u boxer_CGAComponentMode(void)
{
    return static_cast<Bit8u>(cga_comp);
}

// INT-022: Set CGA composite mode
void boxer_setCGAComponentMode(Bit8u newCGA)
{
    if (newCGA > 2) newCGA = 0;  // Invalid → Auto
    cga_comp = static_cast<COMPOSITE_STATE>(newCGA);
    apply_composite_state();
}
```

### Priority 2: Testing Strategy (1-2 hours)

Test each integration point:

1. **Hercules Tinting** (MCH_HERC):
   - Start game in Hercules mode
   - Open View → Hercules Tinting menu
   - Cycle through Amber → Green → White
   - Verify palette changes immediately
   - Check `boxer_herculesTintMode()` returns correct value

2. **CGA Composite Mode** (MCH_CGA):
   - Start game in CGA mode (graphics mode)
   - Open View → CGA Composite Mode menu
   - Test Auto → On → Off → Auto cycle
   - Verify color artifacts appear/disappear
   - Test with games that use composite tricks (King's Quest, etc.)

3. **CGA Hue Adjustment** (MCH_CGA):
   - Enable CGA composite mode (On)
   - Adjust hue slider -360 to +360
   - Verify colors shift smoothly
   - Test extremes (sky should cycle through color spectrum)

4. **Edge Cases**:
   - Send invalid values: `boxer_setHerculesTintMode(99)` → should wrap to valid
   - Send invalid composite: `boxer_setCGAComponentMode(5)` → should default to Auto
   - Rapid changes: Toggle settings quickly, verify no crashes

5. **Machine Type Checks**:
   - Test Hercules functions only affect MCH_HERC
   - Test CGA functions only affect MCH_CGA/MCH_TANDY/MCH_PCJR
   - Verify no crashes when called in wrong machine type

### Priority 3: Documentation (30 min)

Update Boxer documentation:
- Note that Hercules tinting is limited to 3 modes (target has 4)
- Document which CGA features Boxer exposes vs target's full capability
- Note that target has additional composite knobs (saturation, contrast, etc.) that Boxer could expose in future

### Future Enhancement Opportunities

Target DOSBox-staging has enhanced composite simulation features that Boxer doesn't expose yet:

1. **CGA Era Selection** (old-era vs new-era CGA)
2. **Additional Composite Knobs**:
   - Saturation (0-360)
   - Contrast (0-360)
   - Brightness (-100 to 100)
   - Convergence (-50 to 50, controls sharpness)
3. **Paperwhite Hercules Palette** (4th option)

Consider adding these to Boxer's UI for enhanced retro authenticity.

## Blockers/Open Questions

### Resolved

✅ **Do the graphics mode variables still exist?** - YES, with minor type changes
✅ **Is runtime configuration still supported?** - YES, fully supported
✅ **Are the APIs public or internal?** - Mix: some public APIs, some need wrappers
✅ **What data types are used?** - Enums and knob objects instead of raw integers

### None Remaining

No blockers identified. All 6 integration points have clear migration paths with existing target APIs.

---

## Conclusion

Graphics mode support in DOSBox-staging target is **excellent**. All features Boxer needs are present and working, with even better implementations than the legacy code. Migration requires simple wrapper functions (2-4 hours work) with no user-visible changes. This is one of the smoothest integration areas in the entire upgrade project.

**Recommendation**: Proceed with confidence. These 6 integration points are LOW RISK with straightforward solutions.
