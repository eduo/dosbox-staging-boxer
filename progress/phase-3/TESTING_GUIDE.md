# Phase 3: Rendering & Display - Testing Guide

**Last Updated**: 2025-11-15

---

## Overview

This guide walks through building and testing Phase 3 (Rendering) to verify that Boxer can display DOSBox video output using Metal.

**Expected Result**: DOS programs should render on screen with correct colors, aspect ratio, and smooth performance.

---

## Prerequisites

Before testing, ensure you have:

- ✅ Xcode installed (14.0 or later)
- ✅ CMake installed (`brew install cmake`)
- ✅ All Phase 3 code committed to both repositories
- ✅ A DOS program to test with (e.g., DOOM.EXE, EDIT.COM, or any .exe/.com file)

---

## Step 1: Build DOSBox Staging Library

### 1.1 Navigate to DOSBox Staging

```bash
cd /Users/eduo/Developer/dosbox-staging-boxer/boxer-upgrade/src/dosbox-staging
```

### 1.2 Configure CMake with BOXER_INTEGRATED

```bash
cmake -S . -B build-boxer \
  -DCMAKE_BUILD_TYPE=Release \
  -DBOXER_INTEGRATED=ON \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
```

**What this does**:
- Creates build directory `build-boxer/`
- Enables `BOXER_INTEGRATED` flag (activates all our hooks)
- Builds universal binary (Apple Silicon + Intel)
- Targets macOS 11.0+

**Expected output**:
```
-- The C compiler identification is AppleClang ...
-- The CXX compiler identification is AppleClang ...
-- BOXER_INTEGRATED is ON
-- Configuring done
-- Generating done
```

### 1.3 Build the Library

```bash
cmake --build build-boxer --config Release -j$(sysctl -n hw.ncpu)
```

**This will take 5-15 minutes** depending on your Mac.

**Expected output**:
```
[  1%] Building CXX object ...
[  2%] Building CXX object ...
...
[100%] Built target dosbox
```

### 1.4 Verify Library Created

```bash
ls -lh build-boxer/libdosbox.a
```

**Expected output**:
```
-rw-r--r--  1 eduo  staff   XXM Nov 15 23:30 build-boxer/libdosbox.a
```

The library should be several megabytes (5-20 MB).

### 1.5 Verify BOXER_INTEGRATED Hooks

```bash
nm build-boxer/libdosbox.a | grep -i boxer | head -10
```

**Expected output**: Should show Boxer hook references (if not, the hooks are properly inlined, which is fine).

---

## Step 2: Configure Boxer Xcode Project

### 2.1 Open Boxer Project

```bash
cd /Users/eduo/Developer/dosbox-staging-boxer/boxer-upgrade/src/boxer
open Boxer.xcodeproj
```

### 2.2 Add DOSBox Library to Project

In Xcode:

1. **Select Boxer project** in navigator (top-level)
2. **Select "Boxer" target** (not "Bundler")
3. **Build Phases** tab
4. **Link Binary With Libraries** section
5. Click **+** button
6. Click **Add Other...** → **Add Files...**
7. Navigate to: `/Users/eduo/Developer/dosbox-staging-boxer/boxer-upgrade/src/dosbox-staging/build-boxer/`
8. Select **libdosbox.a**
9. Click **Open**

### 2.3 Add Header Search Paths

Still in Build Settings:

1. **Build Settings** tab
2. Search for: **Header Search Paths**
3. Double-click the value
4. Click **+** button
5. Add: `$(SRCROOT)/../dosbox-staging/include`
6. Add: `$(SRCROOT)/../dosbox-staging/src`
7. Both should be **recursive** (checkbox)

### 2.4 Add Preprocessor Macros

In Build Settings:

1. Search for: **Preprocessor Macros**
2. Find **Debug** configuration
3. Add: `BOXER_INTEGRATED=1`
4. Find **Release** configuration  
5. Add: `BOXER_INTEGRATED=1`

### 2.5 Add BXEmulator+BoxerDelegate to Build

In Project Navigator:

1. Find `Boxer/BXEmulator+BoxerDelegate.h`
2. Find `Boxer/BXEmulator+BoxerDelegate.mm`
3. If **not already** in the project:
   - Right-click **Boxer** group
   - **Add Files to "Boxer"...**
   - Select both files
   - ✅ Check **Copy items if needed**
   - ✅ Check **Boxer** target
   - Click **Add**

### 2.6 Verify Compilation Settings

In `BXEmulator+BoxerDelegate.mm`:

1. Select the file in navigator
2. **File Inspector** (right panel)
3. Verify **Type**: Objective-C++ Source
4. Verify **Target Membership**: ✅ Boxer

---

## Step 3: Set Up Boxer Delegate Connection

### 3.1 Initialize Boxer Delegate in BXEmulator

You need to set the global `g_boxer_delegate` pointer when emulation starts.

**Edit**: `Boxer/BXEmulator.mm` (or `BXEmulator.m`)

**Find the method** where DOSBox is initialized (likely in `- (void)start` or similar).

**Add this code** before calling into DOSBox:

```objc
#ifdef BOXER_INTEGRATED
// Set global delegate pointer for DOSBox hooks
extern IBoxerDelegate* g_boxer_delegate;
g_boxer_delegate = static_cast<IBoxerDelegate*>(self);
#endif
```

**And in shutdown/cleanup**:

```objc
#ifdef BOXER_INTEGRATED
// Clear global delegate pointer
extern IBoxerDelegate* g_boxer_delegate;
g_boxer_delegate = nullptr;
#endif
```

### 3.2 Import BoxerDelegate Header

At the top of `BXEmulator.mm`, add:

```objc
#ifdef BOXER_INTEGRATED
#import "BXEmulator+BoxerDelegate.h"
#endif
```

---

## Step 4: Build Boxer

### 4.1 Select Build Scheme

In Xcode:
1. Scheme selector (top toolbar): **Boxer** > **My Mac**
2. Build configuration: **Debug** (for easier debugging)

### 4.2 Build

Press **Cmd+B** or **Product** → **Build**

**Expected**: Build succeeds with no errors

**Common Errors**:

**Error**: `'boxer/boxer_hooks.h' file not found`
- **Fix**: Check Header Search Paths include `../dosbox-staging/include`

**Error**: `Undefined symbols for architecture arm64: _g_boxer_delegate`
- **Fix**: Ensure `libdosbox.a` is in Link Binary With Libraries

**Error**: `Use of undeclared identifier 'IBoxerDelegate'`
- **Fix**: Add `#import "BXEmulator+BoxerDelegate.h"` to BXEmulator.mm

### 4.3 Verify Build Products

After successful build:

```bash
ls -lh ~/Library/Developer/Xcode/DerivedData/Boxer-*/Build/Products/Debug/Boxer.app
```

Should show the built app bundle.

---

## Step 5: Run Basic Test

### 5.1 Launch Boxer

In Xcode:
- Press **Cmd+R** or **Product** → **Run**

**Expected**: Boxer launches normally

**If crashes immediately**:
- Check Console for crash log
- Look for "BOXER ERROR" messages
- Verify delegate pointer is set

### 5.2 Test Without DOS Program First

In Boxer:
1. Create a new gamebox (empty)
2. See if it launches to DOS prompt

**Expected**: 
- DOS prompt appears (C:\>)
- Text is visible and correct color (white on black)
- Cursor blinks
- Keyboard input works (type "DIR", see output)

**Success Criteria**:
- ✅ Text mode renders correctly
- ✅ Cursor visible
- ✅ Keyboard input works
- ✅ Colors correct (white text, black background)

---

## Step 6: Test Video Modes

### 6.1 Test Text Mode (Already Done)

DOS prompt is 80×25 text mode. If that works, text mode is working!

### 6.2 Test Graphics Mode - Simple

At DOS prompt, type:

```
DEBUG
```

Then press Enter. DEBUG has a graphical interface.

**Expected**:
- Graphics render correctly
- No visual corruption
- Mouse works (if DEBUG supports it)

Type `Q` to quit DEBUG.

### 6.3 Test with Actual DOS Program

Copy a DOS game/program to a test gamebox:

**Option A: Use existing game** (if you have one configured in Boxer)

**Option B: Create test gamebox with DOS program**:

1. Create folder: `~/Desktop/TestGame/`
2. Copy a DOS .exe/.com file into it
3. In Boxer: **File** → **Open Gamebox**
4. Select the folder
5. Boxer should detect the .exe and launch it

**Test Programs** (if you have them):
- **DOOM.EXE**: VGA 320×200 (best test!)
- **EDIT.COM**: Text mode editor
- **Prince of Persia**: CGA 320×200
- **Any DOS game you have**

### 6.4 What to Check During Test

While program is running:

**Visual Quality**:
- ✅ Graphics render without corruption
- ✅ Colors look correct (not swapped/inverted)
- ✅ Aspect ratio correct (not stretched wrong)
- ✅ No tearing or flickering
- ✅ Text readable

**Performance**:
- ✅ Smooth animation (no stuttering)
- ✅ 60 FPS or higher (watch frame rate)
- ✅ No lag or freezing

**Interactivity**:
- ✅ Keyboard input works
- ✅ Mouse works (if game uses mouse)
- ✅ Can quit cleanly (Cmd+Q or close window)

**Video Mode Changes**:
- ✅ Mode switches don't crash (e.g., game menu → gameplay)
- ✅ Transitions smooth
- ✅ Resolution changes handled

---

## Step 7: Debug Common Issues

### Issue: Black Screen

**Symptoms**: Boxer launches but screen is black

**Possible Causes**:
1. `startFrame` not returning valid buffer
2. Metal texture not created
3. BXVideoHandler not initialized

**Debug**:
```objc
// Add to BXEmulator+BoxerDelegate.mm startFrame
- (bool) startFrame: (uint8_t**)pixels_out pitch: (int*)pitch_out {
    NSLog(@"🎨 startFrame called");
    void* buffer = nullptr;
    if ([self.videoHandler startFrameWithBuffer: &buffer pitch: pitch_out]) {
        NSLog(@"✅ Buffer: %p, pitch: %d", buffer, *pitch_out);
        *pixels_out = (uint8_t*)buffer;
        return true;
    }
    NSLog(@"❌ startFrameWithBuffer failed");
    return false;
}
```

Rerun and check Console.app for logs.

---

### Issue: Corrupted Graphics

**Symptoms**: Graphics visible but wrong colors/corruption

**Possible Causes**:
1. Pixel format mismatch (RGBA vs BGRA)
2. Pitch/stride incorrect
3. Palette not converted correctly

**Debug**:
```objc
// Check pixel format in getRGBPaletteEntry
- (Bitu) getRGBPaletteEntry: (uint8_t)red green: (uint8_t)green blue: (uint8_t)blue {
    NSLog(@"🎨 Palette: R=%d G=%d B=%d", red, green, blue);
    Bitu result = [self.videoHandler paletteEntryWithRed: red green: green blue: blue];
    NSLog(@"   → 0x%08X", (unsigned int)result);
    return result;
}
```

Expected BGRA32 format: `0xAABBGGRR` (alpha, blue, green, red)

---

### Issue: Crash on Launch

**Symptoms**: Boxer crashes immediately or when starting DOS

**Check**:
1. Is `g_boxer_delegate` set? (Add NSLog before setting it)
2. Is BXVideoHandler initialized? (Check in debugger)
3. Any NULL pointer dereferences?

**Debug**:
- Run in Xcode with debugger (Cmd+R)
- Check where it crashes (should show in Xcode)
- Look at stack trace

**Common crash**: `g_boxer_delegate` is NULL when hook called
**Fix**: Ensure delegate set before DOSBox starts emulation

---

### Issue: Slow Performance / Lag

**Symptoms**: Low FPS, stuttering, lag

**Possible Causes**:
1. Texture upload too slow
2. Event processing blocking
3. Synchronous Metal operations

**Debug**:
```bash
# Run in Instruments (Xcode profiler)
# Product → Profile (Cmd+I)
# Choose "Time Profiler"
```

Look for:
- Long `finishFrame` calls (should be <2ms)
- Long `processEvents` calls (should be <100μs)
- Metal upload bottlenecks

---

### Issue: Input Not Working

**Symptoms**: Keyboard/mouse don't work in DOS

**Check**:
1. Is `processEvents` being called?
2. Are events forwarded to BXEmulatedKeyboard/Mouse?

**Debug**:
```objc
- (bool) processEvents {
    NSLog(@"📥 processEvents called");
    // ... existing code ...
}
```

Should see many logs (called 1000×/sec).

If not logging: Hook not being called, check DOSBox integration.

---

## Step 8: Validation Checklist

Use this checklist to validate Phase 3 success:

### Basic Functionality
- [ ] Boxer launches without crashing
- [ ] DOS prompt appears (text mode)
- [ ] Text is white on black, readable
- [ ] Cursor visible and blinking
- [ ] Keyboard input works (type commands)

### Graphics Rendering
- [ ] DOS program launches and displays
- [ ] Graphics render correctly (no corruption)
- [ ] Colors accurate (not inverted/swapped)
- [ ] Aspect ratio correct (4:3 for most DOS games)
- [ ] No tearing or flickering

### Video Modes
- [ ] Text mode (80×25) works
- [ ] CGA mode (320×200) works (if you test CGA game)
- [ ] EGA mode (640×350) works (if you test EGA game)
- [ ] VGA mode (640×480 or 320×200) works
- [ ] Mode switches don't crash

### Performance
- [ ] 60+ FPS maintained
- [ ] Smooth animation (no stuttering)
- [ ] No dropped frames
- [ ] Input responsive (<50ms latency)

### Interactivity
- [ ] Keyboard works in DOS programs
- [ ] Mouse works (if program uses mouse)
- [ ] Window resize works (optional)
- [ ] Fullscreen works (Cmd+F)
- [ ] Quit works (Cmd+Q or close window)
- [ ] Graceful shutdown (no hang)

### Edge Cases
- [ ] Rapid mode switching works
- [ ] Minimize/restore works
- [ ] Focus change works
- [ ] Long-running program stable (run for 5+ minutes)

---

## Step 9: Performance Testing

### 9.1 Frame Rate Test

Run a demanding DOS game (DOOM if available):

```bash
# In Xcode Console, you should see frame timing
# Add this to finishFrame for testing:
NSLog(@"Frame completed in %.2fms", elapsed_time);
```

**Expected**: Most frames <16ms (60 FPS)

### 9.2 Event Latency Test

Type rapidly in DOS:

**Expected**: No dropped keys, immediate response

### 9.3 Memory Test

Run Activity Monitor while Boxer is running:

**Expected**: 
- Memory stable (not growing continuously)
- CPU <50% on single core
- No memory leaks

---

## Step 10: Report Results

After testing, document your results:

### Success Report Template

```markdown
## Phase 3 Testing Results

**Date**: [Date]
**Tester**: [Your name]
**Mac**: [Model, OS version]

### Build
- [ ] DOSBox library built successfully
- [ ] Boxer built successfully
- [ ] No build errors

### Basic Tests
- [ ] DOS prompt renders: [PASS/FAIL]
- [ ] Keyboard input works: [PASS/FAIL]
- [ ] Colors correct: [PASS/FAIL]

### DOS Programs Tested
1. **[Program name]**: [PASS/FAIL]
   - Video mode: [text/CGA/VGA/etc]
   - Notes: [any issues]

2. **[Program name]**: [PASS/FAIL]
   - Video mode: [text/CGA/VGA/etc]
   - Notes: [any issues]

### Performance
- FPS: [number]
- Smooth: [YES/NO]
- Lag: [YES/NO]

### Issues Found
1. [Issue description]
2. [Issue description]

### Overall Result
- [ ] Phase 3 PASSES all tests ✅
- [ ] Phase 3 has issues (describe below) ⚠️
```

---

## Quick Start (TL;DR)

```bash
# 1. Build DOSBox library
cd src/dosbox-staging
cmake -S . -B build-boxer -DBOXER_INTEGRATED=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-boxer -j8

# 2. Open Boxer in Xcode
cd ../boxer
open Boxer.xcodeproj

# 3. In Xcode:
# - Add libdosbox.a to Link Binary With Libraries
# - Add header search paths
# - Add BOXER_INTEGRATED=1 to preprocessor macros
# - Add delegate initialization code
# - Build (Cmd+B)
# - Run (Cmd+R)

# 4. Test:
# - Launch Boxer
# - Create/open gamebox
# - Run DOS program
# - Verify rendering works
```

---

## Getting Help

If you encounter issues:

1. **Check Console.app** for error messages
2. **Run in Xcode debugger** to see crash location
3. **Add NSLog statements** to trace execution
4. **Review task reports** in `progress/phase-3/tasks/`
5. **Ask Claude** with specific error messages

---

## Success Criteria Summary

Phase 3 is successful if:

✅ DOS programs display on screen  
✅ Graphics render correctly (no corruption)  
✅ All video modes work (text, CGA, EGA, VGA)  
✅ Performance is smooth (60+ FPS)  
✅ Keyboard and mouse work  
✅ No crashes during normal use  

If all checkboxes pass, **Phase 3 is validated!** ✨

---

**Good luck with testing!** 🚀
