# Phase 7: Input, Audio, Graphics - Agent Tasks

**Phase Duration**: Weeks 13-14
**Total Estimated Hours**: 52-78 hours
**Goal**: Remaining subsystems integrated

**Prerequisites**: Phase 6 complete (parport working)

**⚠️ DECISION REQUIRED**: DEC-003 (Paste Buffer Implementation) must be resolved

---

## IMPORTANT: Repository Structure

**Root**: `/home/user/dosbox-staging-boxer/boxer-upgrade/`

**Two SEPARATE Repositories**:
1. DOSBox Staging (`src/dosbox-staging/`) - Branch: `dosbox-boxer-upgrade-dosboxside`
2. Boxer (`src/boxer/`) - Branch: `boxer-dosbox-upgrade-boxerside`

**Phase 7 modifies**: Both DOSBox Staging (input/audio hooks) and Boxer (input handling, MIDI implementation)

---

## PHASE 7 OVERVIEW

By the end of Phase 7, you will have:
- All input methods working (keyboard, mouse, joystick)
- International keyboard layouts functional
- Paste from clipboard works
- Lock keys synchronized
- MIDI passthrough functional
- CGA composite and Hercules modes available

**This phase handles the "everything else" subsystems.**

---

## SUBSYSTEM BREAKDOWN

### Input Handling (40-60 hours)
- Keyboard layout mapping
- Paste buffer implementation
- Lock key synchronization (Caps, Num, Scroll)
- Mouse capture/release
- Joystick axis mapping

### Audio/MIDI (10-14 hours)
- MidiDeviceBoxer adapter
- CoreMIDI passthrough
- Volume control hooks

### Graphics Modes (2-4 hours)
- CGA composite mode support
- Hercules tinting mode
- Special rendering modes

---

## CRITICAL INTEGRATION POINTS

From integration-overview.md:

**Input (16 points)**:
- INT-061: mouseMovedToPoint
- INT-062: mouseButtonPressed/Released
- INT-063: keyboardLayoutForCurrentInputSource
- INT-064: registerLayoutWithName
- INT-065: useKeyboardLayout
- INT-066: keyForScancode
- INT-067: scancodeForKey
- INT-068: sendKeystroke
- INT-069: pasteToShell
- INT-070: clearPasteBuffer
- INT-071: capsLockEnabled
- INT-072: numLockEnabled
- INT-073: scrollLockEnabled
- INT-074: joystickAxisValue

**Audio/MIDI (6 points)**:
- INT-082: MIDIWillRestart
- INT-083: MIDIDidRestart
- INT-084: sendMIDIMessage
- INT-085: listMIDIDestinations
- INT-086: MIDIDestinationAtIndex

**Graphics (6 points)**:
- INT-017: herculesTintMode
- INT-018: setHerculesTintMode
- INT-019: CGACompositeHueOffset
- INT-020: setCGACompositeHueOffset
- INT-021: CGAComponentMode
- INT-022: setCGAComponentMode

---

## TASK 7-1: Input System Analysis

### Context
- **Phase**: 7
- **Estimated Hours**: 6-8 hours
- **Criticality**: MAJOR
- **Risk Level**: MEDIUM

### Objective
Understand how target DOSBox handles input and map to Boxer's system.

### Prerequisites
- [ ] Phase 6 complete
- [ ] DEC-003 resolved (paste buffer approach)

### Input Documents
1. `src/dosbox-staging/src/gui/sdlmain.cpp`
   - SDL2 input handling
   - Keyboard mapping

2. `src/boxer/Boxer/BXEmulatedKeyboard.mm`
   - Current keyboard implementation

3. `src/boxer/Boxer/BXEmulatedMouse.mm`
   - Current mouse implementation

### Deliverables
1. **Analysis**: `progress/phase-7/INPUT_SYSTEM_ANALYSIS.md`
   - Keyboard scancode mapping
   - Mouse event flow
   - Joystick API comparison
   
2. **Documentation**: `progress/phase-7/tasks/TASK-7-1.md`

### Key Questions
1. How does target DOSBox receive keyboard events?
2. How are scancodes mapped to keycodes?
3. How is mouse capture handled?
4. Where are joystick events processed?

### Success Criteria
- [ ] Complete input system understanding
- [ ] Integration strategy defined
- [ ] No blocking issues found

---

## TASK 7-2: Keyboard Layout Hooks

### Context
- **Phase**: 7
- **Estimated Hours**: 12-16 hours
- **Criticality**: MAJOR
- **Risk Level**: MEDIUM

### Objective
Integrate keyboard layout management (international keyboards).

### Prerequisites
- [ ] TASK 7-1 complete (input analysis done)

### Deliverables
1. **Modified**: `src/dosbox-staging/src/dos/dos_keyboard.cpp` (or similar)
   - Add hooks for keyboard layout queries
   - Allow Boxer to provide layout information
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement keyboardLayoutForCurrentInputSource()
   - Implement registerLayoutWithName()
   - Implement useKeyboardLayout()
   - Implement keyForScancode()
   - Implement scancodeForKey()
   
3. **Test**: International keyboard test
   
4. **Documentation**: `progress/phase-7/tasks/TASK-7-2.md`

### Keyboard Layout Features
- Auto-detect macOS input source
- Map to DOS keyboard layout
- Support custom layouts
- Handle dead keys and modifiers

### Success Criteria
- [ ] Correct characters typed
- [ ] Special keys work (accents, umlauts)
- [ ] Layout switching works
- [ ] US, UK, German, French, Spanish layouts work

---

## TASK 7-3: Paste Buffer Implementation

### Context
- **Phase**: 7
- **Estimated Hours**: 12-18 hours
- **Criticality**: MAJOR
- **Risk Level**: MEDIUM

### Objective
Implement clipboard paste functionality (per DEC-003 decision).

### Prerequisites
- [ ] TASK 7-2 complete (keyboard works)
- [ ] DEC-003 resolved

### Deliverables (for Boxer-Side implementation - Option A)

1. **Modified**: `src/boxer/Boxer/BXEmulatedKeyboard.mm`
   - Queue clipboard text as keystrokes
   - Convert characters to scancodes
   - Handle pacing (not too fast for DOS)
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement pasteToShell()
   - Implement clearPasteBuffer()
   
3. **Test**: Paste test
   
4. **Documentation**: `progress/phase-7/tasks/TASK-7-3.md`

### Implementation (Boxer-Side)

```objc
// In BXEmulatedKeyboard.mm
- (void)pasteString:(NSString *)text {
    // Queue each character
    for (NSUInteger i = 0; i < text.length; i++) {
        unichar c = [text characterAtIndex:i];
        uint8_t scancode = [self scancodeForCharacter:c];
        [self queueKeystroke:scancode];
    }
    
    // Inject keystrokes at appropriate rate
    [self startKeystrokeInjection];
}
```

### Success Criteria
- [ ] Text pastes correctly
- [ ] Special characters handled
- [ ] Pasting doesn't overwhelm DOS
- [ ] Large pastes work
- [ ] Clear buffer works

---

## TASK 7-4: Lock Key Synchronization

### Context
- **Phase**: 7
- **Estimated Hours**: 6-8 hours
- **Criticality**: MINOR
- **Risk Level**: LOW

### Objective
Synchronize Caps Lock, Num Lock, Scroll Lock between macOS and DOS.

### Prerequisites
- [ ] TASK 7-3 complete (keyboard working)

### Deliverables
1. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement capsLockEnabled()
   - Implement numLockEnabled()
   - Implement scrollLockEnabled()
   - Query macOS key states
   
2. **Modified**: DOSBox keyboard code
   - Poll Boxer for lock key states
   
3. **Documentation**: `progress/phase-7/tasks/TASK-7-4.md`

### Implementation

```objc
bool BoxerDelegateImpl::capsLockEnabled() {
    NSUInteger flags = [NSEvent modifierFlags];
    return (flags & NSEventModifierFlagCapsLock) != 0;
}
```

### Success Criteria
- [ ] DOS reflects macOS Caps Lock state
- [ ] Num Lock synchronized
- [ ] Scroll Lock synchronized
- [ ] No race conditions

---

## TASK 7-5: Mouse and Joystick Integration

### Context
- **Phase**: 7
- **Estimated Hours**: 8-12 hours
- **Criticality**: MAJOR
- **Risk Level**: LOW

### Objective
Complete mouse capture/release and joystick axis mapping.

### Prerequisites
- [ ] TASK 7-4 complete (lock keys work)

### Deliverables
1. **Modified**: DOSBox mouse handling
   - Add hooks for mouse position
   - Handle mouse capture state
   
2. **Modified**: DOSBox joystick handling
   - Add hooks for axis values
   - Map to macOS game controller
   
3. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement mouseMovedToPoint()
   - Implement mouseButtonPressed/Released()
   - Implement joystickAxisValue()
   
4. **Documentation**: `progress/phase-7/tasks/TASK-7-5.md`

### Success Criteria
- [ ] Mouse works in DOS games
- [ ] Mouse capture toggles correctly
- [ ] Joystick axes mapped
- [ ] Analog controls smooth

---

## TASK 7-6: MIDI Device Implementation

### Context
- **Phase**: 7
- **Estimated Hours**: 10-14 hours
- **Criticality**: MINOR
- **Risk Level**: LOW

### Objective
Implement MidiDeviceBoxer adapter for CoreMIDI output.

### Prerequisites
- [ ] TASK 7-5 complete (input done)

### Input Documents
1. `analysis/03-reintegration-analysis/unavoidable-modifications.md` lines 686-790
   - MidiDeviceBoxer design

2. `src/dosbox-staging/src/hardware/midi/`
   - MIDI device architecture

### Deliverables
1. **New file**: `src/dosbox-staging/src/hardware/midi/midi_boxer.h`
   - MidiDeviceBoxer class declaration
   
2. **New file**: `src/dosbox-staging/src/hardware/midi/midi_boxer.cpp`
   - MidiDevice interface implementation
   
3. **Modified**: `src/boxer/Boxer/BXMIDIDevice.m`
   - Handle MIDI messages from DOSBox
   - Route to CoreMIDI
   
4. **Test**: MIDI playback test
   
5. **Documentation**: `progress/phase-7/tasks/TASK-7-6.md`

### Implementation

```cpp
// midi_boxer.h
class MidiDeviceBoxer final : public MidiDevice {
public:
    bool Open(const char* conf) override;
    void Close() override;
    void SendMidiMessage(const MidiMessage& msg) override;
    void SendSysExMessage(uint8_t* data, size_t length) override;
};
```

### Success Criteria
- [ ] MIDI device registerable
- [ ] MIDI messages sent to Boxer
- [ ] CoreMIDI receives messages
- [ ] Music plays correctly
- [ ] No audio glitches

---

## TASK 7-7: Special Graphics Modes

### Context
- **Phase**: 7
- **Estimated Hours**: 2-4 hours
- **Criticality**: MINOR
- **Risk Level**: VERY LOW

### Objective
Support CGA composite and Hercules tinting modes.

### Prerequisites
- [ ] TASK 7-6 complete (MIDI done)

### Deliverables
1. **Modified**: `src/dosbox-staging/src/hardware/vga_other.cpp`
   - Add hooks for graphics mode queries
   - Allow Boxer to set modes
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement herculesTintMode()
   - Implement CGACompositeHueOffset()
   - etc.
   
3. **Documentation**: `progress/phase-7/tasks/TASK-7-7.md`

### Success Criteria
- [ ] Hercules tinting adjustable
- [ ] CGA composite colors configurable
- [ ] Modes persist across sessions

---

## PHASE 7 COMPLETION CHECKLIST

### Input Handling ✅
- [ ] Keyboard layout management works
- [ ] International characters correct
- [ ] Paste buffer functional
- [ ] Lock keys synchronized
- [ ] Mouse capture works
- [ ] Joystick axes mapped

### Audio/MIDI ✅
- [ ] MIDI device adapter works
- [ ] CoreMIDI integration functional
- [ ] Music plays correctly

### Graphics Modes ✅
- [ ] Hercules tinting works
- [ ] CGA composite configurable

### Integration ✅
- [ ] All 28 hooks functional
- [ ] No regressions
- [ ] Performance acceptable

**When all boxes checked, Phase 7 is complete. Ready for Phase 8 (Testing).**

---

## ESTIMATED TIME BREAKDOWN

- TASK 7-1: Input System Analysis - 6-8 hours
- TASK 7-2: Keyboard Layout Hooks - 12-16 hours
- TASK 7-3: Paste Buffer - 12-18 hours
- TASK 7-4: Lock Key Sync - 6-8 hours
- TASK 7-5: Mouse/Joystick - 8-12 hours
- TASK 7-6: MIDI Device - 10-14 hours
- TASK 7-7: Graphics Modes - 2-4 hours

**Total**: 56-80 hours (~52-78 planned)

**Calendar time**: 1.5-2 weeks