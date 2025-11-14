# Audio & MIDI Integration Analysis

**Agent**: Agent 1B.7 - Audio & MIDI
**Created**: 2025-11-14T00:00:00Z
**Status**: Completed
**Dependencies**: Agent 1A

## Summary

The audio and MIDI integration points have undergone significant architectural changes in the target DOSBox. The MIDI system has been refactored from direct callback functions to an object-oriented MidiDevice interface with byte-stream processing. The mixer volume system has evolved from simple float arrays to a thread-safe AudioFrame structure with separate user/app/scalar volume gains. All 6 integration points exist in some form but require adaptation - 2 have signature changes, 4 have been architecturally refactored. Migration effort: 8-12 hours including testing.

## Legacy Audio Architecture

### MIDI System

**Handler Architecture:**
- Legacy uses direct callback functions to Boxer via `BXCoalfaceAudio.h`
- MIDI messages processed in `MIDI_RawOutByte()` and sent via callbacks
- Handler selection done programmatically via `boxer_suggestMIDIHandler()`
- Simple availability check via `boxer_MIDIAvailable()` macro

**Message Flow:**
1. `MIDI_RawOutByte()` receives raw MIDI bytes
2. Bytes assembled into complete messages or SysEx streams
3. Complete messages sent directly to Boxer callbacks:
   - `boxer_sendMIDIMessage(midi.rt_buf)` - regular messages
   - `boxer_sendMIDISysex(midi.sysex.buf, midi.sysex.used)` - SysEx

**Files:**
- `/home/user/dosbox-staging-boxer/src/midi/midi.cpp` (347 lines)
- `/home/user/Boxer/Boxer/BXCoalfaceAudio.h` - Boxer interface

### Audio Mixer

**Mixing Architecture:**
- Single global mixer struct with `mastervol[2]` float array (line 104)
- Channels linked list with individual volume multipliers
- `UpdateVolume()` per channel multiplies scale × level × mastervol

**Volume Control:**
- Master volume: `mixer.mastervol[0]` (left), `mixer.mastervol[1]` (right)
- Per-channel: `volmain[2]` and `scale[2]` arrays
- Final volume: `volmul = scale × level × mastervol` (lines 183-184)

**Files:**
- `/home/user/dosbox-staging-boxer/src/hardware/mixer.cpp` (962 lines)

### Boxer Integration

**MIDI Routing:**
- Boxer receives MIDI messages via callbacks in `BXCoalfaceAudio.h`
- Routes to macOS CoreMIDI or internal synthesizers
- Handler type suggested by DOSBox, final decision by Boxer

**Audio Routing:**
- Boxer queries master volume via `boxer_masterVolume(BXAudioChannel)`
- Notified of volume changes via `boxer_updateVolumes()`
- Uses volumes to mix DOSBox audio with host audio system

## Target Audio Architecture

### System Changes

**Major Differences:**
1. MIDI moved from direct callbacks to object-oriented MidiDevice interface
2. Mixer now uses AudioFrame struct instead of float arrays
3. Volume system expanded to user/app/scalar gain separation
4. Thread-safe atomic operations for master volume
5. Configuration-based MIDI device selection

**New Capabilities:**
- Multiple MIDI device types (FluidSynth, MT-32, SoundCanvas, CoreMIDI, etc.)
- Sophisticated MIDI stream sanitization
- Separate user and application volume controls
- Reverb, chorus, crossfeed effects integration

### MIDI Handler System

**Device Architecture:**
- Abstract `MidiDevice` base class with `SendMidiMessage()` and `SendSysExMessage()`
- Concrete implementations: CoreMIDI, CoreAudio, FluidSynth, MT-32, SoundCanvas, etc.
- Device created via factory function `create_device(name, config)`
- Global `midi` struct holds current device pointer

**Message Processing:**
1. `MIDI_RawOutByte(uint8_t data)` - single entry point (line 361)
2. Bytes assembled into `MidiMessage` structs
3. Messages processed through state tracker and sanitizer
4. Sent to device: `midi.device->SendMidiMessage(msg)`
5. SysEx: `midi.device->SendSysExMessage(buf, len)`

**Handler Selection:**
- Configuration-based: `mididevice` setting in `[midi]` section
- Defaults to "port" (system MIDI interface)
- No programmatic suggestion API - config only

**Files:**
- `/home/user/dosbox-staging/src/midi/midi.cpp` (952 lines)
- `/home/user/dosbox-staging/src/midi/private/midi_device.h` (44 lines)

### Mixer Architecture

**Volume System:**
- `AudioFrame` struct: `{float left, float right}`
- Three volume components per channel:
  - `user_volume_gain` - user-controlled via MIXER command
  - `app_volume_gain` - application-controlled (clamped 0-1)
  - `db0_volume_gain` - scalar for 0dB mapping
  - `combined_volume_gain` = user × app × db0
- Master volume: atomic `AudioFrame` (lines 843-851)

**API:**
- `MIXER_GetMasterVolume()` → `AudioFrame`
- `MIXER_SetMasterVolume(AudioFrame gain)` - atomic
- `MixerChannel::GetUserVolume()` → `AudioFrame`
- `MixerChannel::SetUserVolume(AudioFrame gain)` - calls `UpdateCombinedVolume()`
- `MixerChannel::UpdateCombinedVolume()` - recalculates combined gain

**Thread Safety:**
- Master volume uses `std::atomic<AudioFrame>`
- Channel volumes protected by `std::recursive_mutex`
- Lock-free reads, locked writes

**Files:**
- `/home/user/dosbox-staging/src/audio/mixer.cpp` (33K+ lines)
- `/home/user/dosbox-staging/src/audio/mixer.h` (extensive API)

## Integration Point Analysis

### INT-014: boxer_MIDIAvailable

**Legacy Implementation:**
- **Location**: `/home/user/dosbox-staging-boxer/src/midi/midi.cpp` (commented out lines 226-229)
- **Purpose**: Check if MIDI system is available
- **Signature**: `bool boxer_MIDIAvailable(void)` (macro in BXCoalfaceAudio.h:20)
- **Usage**: Called by Boxer to determine MIDI availability

**Target Equivalent:**
- **Status**: EXISTS (renamed)
- **Location**: `/home/user/dosbox-staging/src/midi/midi.cpp:589-592`
- **Function**: `bool MIDI_IsAvailable()`
- **Implementation**: `return (midi.device != nullptr);`

**Compatibility**: SIGNATURE (function name changed)

**Migration Strategy**:
- Replace `boxer_MIDIAvailable()` with call to `MIDI_IsAvailable()`
- Simple 1:1 function rename
- No behavioral changes

---

### INT-082: boxer_sendMIDIMessage

**Legacy Implementation:**
- **Location**: `/home/user/dosbox-staging-boxer/src/midi/midi.cpp:156,218`
- **Purpose**: Send MIDI channel message to Boxer
- **Signature**: `void boxer_sendMIDIMessage(Bit8u *msg)` (BXCoalfaceAudio.h:23)
- **Usage**: Called when complete MIDI message assembled
- **Data**: Pointer to 1-3 byte message buffer

**Target Equivalent:**
- **Status**: REFACTORED
- **Location**: No direct equivalent - integrated into MIDI_RawOutByte
- **Mechanism**:
  - Messages assembled in `midi.message.msg` (MidiMessage struct)
  - Sent via `midi.device->SendMidiMessage(midi.message.msg)` (line 512)
  - Device interface: `MidiDevice::SendMidiMessage(const MidiMessage& msg)` (midi_device.h:34)

**Compatibility**: REFACTORED

**Migration Strategy**:
- Cannot intercept at same point - messages sent internally
- Options:
  1. Create custom MidiDevice subclass that forwards to Boxer
  2. Hook MIDI_RawOutByte to intercept before processing
  3. Add callback registration to MIDI system
- Recommended: Custom MidiDevice implementation "MidiDeviceBoxer"

---

### INT-083: boxer_sendMIDISysex

**Legacy Implementation:**
- **Location**: `/home/user/dosbox-staging-boxer/src/midi/midi.cpp:175`
- **Purpose**: Send MIDI System Exclusive message to Boxer
- **Signature**: `void boxer_sendMIDISysex(Bit8u *msg, Bitu len)` (BXCoalfaceAudio.h:24)
- **Usage**: Called when complete SysEx message received
- **Data**: Buffer pointer and length (up to 20KB)

**Target Equivalent:**
- **Status**: REFACTORED
- **Location**: No direct equivalent - integrated into MIDI_RawOutByte
- **Mechanism**:
  - SysEx assembled in `midi.sysex.buf[]` array
  - Sent via `midi.device->SendSysExMessage(midi.sysex.buf, midi.sysex.pos)` (line 409)
  - Device interface: `MidiDevice::SendSysExMessage(uint8_t* sysex, size_t len)` (midi_device.h:36-37)

**Compatibility**: REFACTORED

**Migration Strategy**:
- Same as INT-082 - custom MidiDevice subclass
- Implement both `SendMidiMessage()` and `SendSysExMessage()` methods
- Forward calls to Boxer via BXCoalfaceAudio

---

### INT-084: boxer_suggestMIDIHandler

**Legacy Implementation:**
- **Location**: `/home/user/dosbox-staging-boxer/src/midi/midi.cpp:243`
- **Purpose**: Notify Boxer of DOSBox MIDI handler preference
- **Signature**: `void boxer_suggestMIDIHandler(std::string const &handlerName, const char *configParams)` (BXCoalfaceAudio.h:17)
- **Usage**: Called during MIDI initialization with config values
- **Data**: Handler name ("auto", "coremidi", etc.) and config string

**Target Equivalent:**
- **Status**: REFACTORED
- **Location**: Configuration system only
- **Mechanism**:
  - MIDI device selected via `mididevice` setting in `[midi]` section
  - `MIDI_Init()` reads config and creates appropriate device (line 736-783)
  - No programmatic suggestion API - all config-driven
  - Device preference: "port" (default), "coreaudio", "mt32", "soundcanvas", "fluidsynth", "none"

**Compatibility**: REFACTORED

**Migration Strategy**:
- Boxer needs to set `mididevice` config before DOSBox MIDI initialization
- Use config system: `set_section_property_value("midi", "mididevice", value)`
- Cannot suggest dynamically after init - requires MIDI_Init() restart
- Alternative: Boxer could provide default via config injection

---

### INT-085: boxer_masterVolume

**Legacy Implementation:**
- **Location**: `/home/user/dosbox-staging-boxer/src/hardware/mixer.cpp:183,184,803`
- **Purpose**: Query current master volume for left or right channel
- **Signature**: `float boxer_masterVolume(BXAudioChannel channel)` (BXCoalfaceAudio.h:26)
  - `BXAudioChannel` enum: `BXLeftChannel = 0`, `BXRightChannel = 1`
- **Usage**: Called during volume calculation and MIXER command display
- **Returns**: Volume as float (0.0 to 1.0+)

**Target Equivalent:**
- **Status**: EXISTS (signature changed)
- **Location**: `/home/user/dosbox-staging/src/audio/mixer.cpp:843-846`
- **Function**: `const AudioFrame MIXER_GetMasterVolume()`
- **Implementation**: `return mixer.master_gain.load(std::memory_order_relaxed);`
- **Returns**: `AudioFrame` struct with `.left` and `.right` float members
- **Thread Safety**: Atomic read operation

**Compatibility**: SIGNATURE (return type changed from per-channel float to struct)

**Migration Strategy**:
- Replace `boxer_masterVolume(BXLeftChannel)` with `MIXER_GetMasterVolume().left`
- Replace `boxer_masterVolume(BXRightChannel)` with `MIXER_GetMasterVolume().right`
- Simple accessor change, same semantic meaning

---

### INT-086: boxer_updateVolumes

**Legacy Implementation:**
- **Location**: `/home/user/dosbox-staging-boxer/src/hardware/mixer.cpp:952-960`
- **Purpose**: Notify all channels to recalculate volumes after master change
- **Signature**: `void boxer_updateVolumes()` (BXCoalfaceAudio.h:29)
- **Usage**: Called by Boxer when master volume changes
- **Implementation**: Iterates all channels calling `channel->UpdateVolume()`

**Target Equivalent:**
- **Status**: REFACTORED
- **Location**: No global update function
- **Mechanism**:
  - Each channel has `MixerChannel::UpdateCombinedVolume()` (line 792-796)
  - Master volume is atomic - channels read on-demand during mixing
  - No notification system - pull model instead of push
  - Channels multiply by `mixer.master_gain` during mix (line 1911)

**Compatibility**: REFACTORED

**Migration Strategy**:
- Not needed in target architecture - channels read master volume atomically
- If Boxer needs to force update, can iterate channels manually:
  ```cpp
  // Pseudo-code
  for (auto& [name, channel] : mixer.channels) {
      channel->UpdateCombinedVolume();
  }
  ```
- However, this should not be necessary - atomic master volume is read during mix

---

## Summary Table

| ID | Name | System | Target Status | Compatibility | Complexity |
|----|------|--------|---------------|---------------|------------|
| INT-014 | MIDIAvailable | MIDI | EXISTS | SIGNATURE | Low (1h) |
| INT-082 | sendMIDIMessage | MIDI | REFACTORED | REFACTORED | High (3-4h) |
| INT-083 | sendMIDISysex | MIDI | REFACTORED | REFACTORED | High (3-4h) |
| INT-084 | suggestMIDIHandler | MIDI | REFACTORED | REFACTORED | Medium (2h) |
| INT-085 | masterVolume | Mixer | EXISTS | SIGNATURE | Low (1h) |
| INT-086 | updateVolumes | Mixer | REFACTORED | REFACTORED | Low-Med (1-2h) |

## Audio System Compatibility

### MIDI Routing

**Status**: Needs Major Adaptation

**Changes Required:**
1. **Create Custom MidiDevice**: Implement `MidiDeviceBoxer` class
   - Inherit from `MidiDevice` base class
   - Implement `SendMidiMessage(const MidiMessage& msg)`
   - Implement `SendSysExMessage(uint8_t* sysex, size_t len)`
   - Forward calls to Boxer via existing BXCoalfaceAudio callbacks

2. **Register Device**: Add to MIDI device factory
   - Modify `create_device()` to recognize "boxer" device type
   - Add to available devices list in config

3. **Config Integration**: Default to Boxer device
   - Set `mididevice = boxer` in Boxer's DOSBox config
   - Maintain existing Boxer MIDI routing logic

**Migration Approach:**
```cpp
// New file: src/midi/private/boxer.h
class MidiDeviceBoxer : public MidiDevice {
public:
    std::string GetName() const override { return "boxer"; }
    Type GetType() const override { return Type::External; }

    void SendMidiMessage(const MidiMessage& msg) override {
        boxer_sendMIDIMessage(const_cast<uint8_t*>(msg.data.data()));
    }

    void SendSysExMessage(uint8_t* sysex, size_t len) override {
        boxer_sendMIDISysex(sysex, len);
    }
};
```

### Volume Control

**Status**: Compatible with Minor Changes

**Changes Required:**
1. **Update master volume queries**: Replace per-channel function with struct accessor
2. **Remove update notification**: Not needed with atomic master volume
3. **Consider volume persistence**: Target has volume caching in `channel_settings_cache`

**Migration Approach:**
- Replace `boxer_masterVolume(channel)` calls with `MIXER_GetMasterVolume().left/right`
- Remove `boxer_updateVolumes()` calls - no longer needed
- Volume changes via `MIXER_SetMasterVolume(AudioFrame{left, right})`

### Handler Selection

**Status**: Major Changes - Config-Based Instead of Programmatic

**Changes Required:**
1. **Pre-configure MIDI device**: Set before DOSBox init, not during
2. **Remove suggestion callback**: No equivalent in target
3. **Use config system**: Manipulate `mididevice` setting directly

**Migration Approach:**
- Boxer analyzes game requirements before launching DOSBox
- Sets `mididevice` config appropriately (or uses custom "boxer" device)
- No runtime suggestion - decision made at config time

## Migration Complexity

**Total Effort**: 10-14 hours

**Breakdown:**
- MIDI integration: 6-8 hours
  - Custom MidiDevice implementation: 3-4h
  - Config integration: 1-2h
  - Testing MIDI routing: 2h
- Mixer integration: 2-3 hours
  - Volume API updates: 1-2h
  - Remove update notifications: 0.5h
  - Testing volume control: 1h
- Integration testing: 2-3 hours
  - MIDI output verification
  - Volume synchronization
  - Edge cases (mute, device switching)

## Risks

### MEDIUM Risks

**MIDI Device Initialization Order:**
- Target MIDI system initializes during config load
- Boxer may need MIDI routing before DOSBox is fully initialized
- Mitigation: Ensure Boxer's MIDI device is created early, or use lazy init

**Volume Precision:**
- Legacy uses float arrays, target uses AudioFrame struct
- Potential precision differences in volume calculations
- Mitigation: Verify volume ranges match (0.0-1.0+), test extreme values

**Thread Safety:**
- Target mixer is thread-safe with atomics and mutexes
- Boxer callbacks must be thread-safe if called from mixer thread
- Mitigation: Document thread context, add locks if needed

### LOW Risks

**MIDI Message Format:**
- Target uses `MidiMessage` struct vs raw `Bit8u*` arrays
- Simple conversion: `msg.data.data()` provides raw pointer
- Very low risk - well-defined interface

**Volume Notification Removal:**
- No longer notified when master volume changes
- Boxer would need to poll or receive notification differently
- Mitigation: Atomic reads are cheap, poll during mix if needed

## Recommendations

### Priority Actions

1. **Implement MidiDeviceBoxer First** (Week 1)
   - Create custom MIDI device class
   - Integrate with device factory
   - Test with simple MIDI games (Sierra, LucasArts)
   - This unblocks MIDI functionality completely

2. **Update Volume API** (Week 1)
   - Simple find/replace for master volume queries
   - Remove update volume calls
   - Test with games that change volume (CD audio, SB mixer)

3. **Config System Integration** (Week 2)
   - Ensure Boxer sets `mididevice = boxer` in config
   - Test device selection fallback behavior
   - Verify no conflicts with existing Boxer MIDI preferences

### Testing Strategy

**MIDI Testing:**
- Games with heavy MIDI use: Monkey Island, King's Quest 6, X-Wing
- SysEx testing: MT-32 games (Sierra catalog)
- Edge cases: MIDI device switching, mute/unmute
- Verify all message types: Note On/Off, Program Change, Control Change, SysEx

**Volume Testing:**
- Master volume changes via MIXER command
- Per-game volume adjustments
- Volume ranges: 0%, 50%, 100%, >100%
- Verify Boxer UI reflects DOSBox volumes correctly

**Integration Testing:**
- Launch multiple games in sequence
- Switch between MIDI devices (MT-32, General MIDI, etc.)
- Volume persistence across game launches
- Audio/MIDI synchronization

### Alternative Approaches

**If Custom MidiDevice Doesn't Work:**
1. **Hook MIDI_RawOutByte**: Intercept at byte level before processing
   - More invasive but guaranteed to catch all MIDI traffic
   - Would require patching target DOSBox

2. **Proxy All MIDI Devices**: Wrap each device type with Boxer forwarder
   - More complex but preserves device switching
   - Boxer sees all MIDI regardless of device type

3. **External MIDI Routing**: Use system MIDI ports
   - Boxer creates virtual MIDI port
   - DOSBox sends to "port" device → Boxer's virtual port
   - Cleaner separation but adds system dependency

**If Volume API Issues Arise:**
1. **Cache Master Volume**: Store copy in Boxer on change notifications
   - Subscribe to config change events
   - Less elegant but works if polling is problematic

2. **Modify Mixer to Add Notifications**: Patch target to add callback
   - More invasive but restores exact legacy behavior
   - Only if atomic reads prove insufficient

## Blockers/Open Questions

**RESOLVED:**
- ✓ MIDI message routing mechanism - MidiDevice interface
- ✓ Volume API structure - AudioFrame with atomic operations
- ✓ Handler selection process - config-based

**REMAINING:**

1. **Thread Context for MIDI Callbacks:**
   - In which thread context are MidiDevice methods called?
   - Does Boxer need thread-safe implementations of callbacks?
   - Action: Review mixer thread architecture in target

2. **Volume Change Events:**
   - How does Boxer detect when DOSBox volume changes?
   - Should Boxer poll, or does target provide events?
   - Action: Check if config update notifications exist

3. **MIDI Device Lifecycle:**
   - When is MidiDevice created/destroyed?
   - Can Boxer create device before DOSBox MIDI_Init?
   - Action: Trace initialization sequence in target

4. **Backward Compatibility:**
   - Should Boxer maintain ability to run legacy DOSBox?
   - If yes, need conditional compilation for both APIs
   - Action: Clarify Boxer's version support requirements

## Next Steps

1. **Agent 1C.7**: Create `MidiDeviceBoxer` implementation stub
2. **Agent 1C.7**: Update all `boxer_masterVolume` call sites
3. **Agent 2A**: Identify MIDI device initialization sequence
4. **Agent 2B**: Map volume notification alternatives
5. **Agent 3A**: Comprehensive MIDI routing tests
