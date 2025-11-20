# Integration Verification Guide

**Version:** 1.0
**Date:** 2025-11-17
**Purpose:** Verify that Boxer is using the NEW DOSBox integration, not the old one

## The Problem

When you have both old and new DOSBox integrations in parallel, you need to be absolutely certain which one is being compiled and run. This guide provides multiple verification methods to confirm you're testing the new integration.

---

## Method 1: Version Stamping (Recommended - Add This First)

### Step 1.1: Add Version Identifier to BXCoalface-New.h

Edit `Boxer/BXCoalface-New.h`:

```cpp
// At the top of the file, after includes
#ifndef DOSBOX_INTEGRATION_VERSION
#define DOSBOX_INTEGRATION_VERSION "NEW-0.83.0"
#endif

// Add a version query function
#ifdef __cplusplus
extern "C" {
#endif

const char* boxer_getDOSBoxIntegrationVersion();

#ifdef __cplusplus
}
#endif
```

Edit `Boxer/BXCoalface-New.mm`:

```objc
const char* boxer_getDOSBoxIntegrationVersion() {
    return DOSBOX_INTEGRATION_VERSION " (New Integration)";
}
```

### Step 1.2: Add Version Identifier to Old BXCoalface.h

Edit `Boxer/BXCoalface.h` (the old one):

```cpp
// At the top
#ifndef DOSBOX_INTEGRATION_VERSION
#define DOSBOX_INTEGRATION_VERSION "OLD-0.78.0"
#endif

// Add same function
const char* boxer_getDOSBoxIntegrationVersion();
```

Edit `Boxer/BXCoalface.mm` (the old one):

```objc
const char* boxer_getDOSBoxIntegrationVersion() {
    return DOSBOX_INTEGRATION_VERSION " (Old Integration)";
}
```

### Step 1.3: Add Version Display in Boxer Startup

Edit `BXEmulator.mm` (in the initialization method):

```objc
- (void)didFinishLaunching {
    // ... existing code ...

    // VERIFICATION: Display which DOSBox integration is being used
    const char* version = boxer_getDOSBoxIntegrationVersion();
    NSLog(@"===========================================");
    NSLog(@"DOSBox Integration Version: %s", version);
    NSLog(@"===========================================");

    // ... rest of initialization ...
}
```

### Step 1.4: Verification

```bash
# Build with new DOSBox scheme
# Select: "Boxer (New DOSBox)" in Xcode

# Run Boxer
open build/Boxer.app

# Check console output (Console.app or Xcode console):
# Should see:
===========================================
DOSBox Integration Version: NEW-0.83.0 (New Integration)
===========================================

# If you see:
DOSBox Integration Version: OLD-0.78.0 (Old Integration)
# Then you're NOT using the new integration - something is misconfigured
```

---

## Method 2: Compilation Verification (Build Log Analysis)

### Step 2.1: Enable Detailed Build Logging

In Xcode:
1. Product → Scheme → Edit Scheme
2. Select "Build"
3. Uncheck "Hide shell script phases"
4. Check "Find implicit dependencies"

### Step 2.2: Build and Capture Log

```bash
# Build from command line with verbose output
cd ~/boxer-upgrade/Boxer

xcodebuild -project Boxer.xcodeproj \
           -scheme "Boxer (New DOSBox)" \
           -configuration Debug-New-DOSBox \
           clean build \
           2>&1 | tee build-verification.log
```

### Step 2.3: Verify Correct Files Being Compiled

```bash
# Check which DOSBox source directory is being compiled
grep -o "DOSBox-Staging[^/]*/" build-verification.log | sort | uniq -c

# Should show:
#   150 DOSBox-Staging-New/    ← NEW version being compiled
#     0 DOSBox-Staging/         ← OLD version NOT compiled

# If you see DOSBox-Staging/ being compiled, you're using the old version!

# Check for new integration files
grep "BXCoalface-New" build-verification.log

# Should see compilation of:
# - BXCoalface-New.mm
# - BXCoalfaceAudio-New.mm

# Check preprocessor defines
grep "DOSBOX_NEW_INTEGRATION" build-verification.log

# Should see:
# -DDOSBOX_NEW_INTEGRATION=1
```

### Step 2.4: Verify Include Paths

```bash
# Extract header search paths
grep "HEADER_SEARCH_PATHS" build-verification.log

# Should include:
# - DOSBox-Staging-New/src
# - DOSBox-Staging-New/include
# Should NOT include (or should come after):
# - DOSBox-Staging/src
```

---

## Method 3: Runtime Callback Verification

### Step 3.1: Add Logging to Critical Callbacks

Edit `BXCoalface-New.mm`:

```objc
bool boxer_runLoopShouldContinue() {
    static bool logged = false;
    if (!logged) {
        NSLog(@"✅ NEW INTEGRATION: boxer_runLoopShouldContinue called");
        logged = true;
    }

    BXEmulator *emulator = [BXEmulator currentEmulator];
    return [emulator _runLoopShouldContinue];
}

uint8_t boxer_prepareForFrameSize(int width, int height,
                                    Fraction aspect_ratio,  // NEW signature
                                    uint8_t flags,
                                    VideoMode& mode,
                                    GFX_Callback_t callback) {
    NSLog(@"✅ NEW INTEGRATION: boxer_prepareForFrameSize called with Fraction aspect");

    float aspect_float = FractionToFloat(aspect_ratio);
    // ... rest of implementation ...
}
```

Edit old `BXCoalface.mm`:

```objc
bool boxer_runLoopShouldContinue() {
    static bool logged = false;
    if (!logged) {
        NSLog(@"❌ OLD INTEGRATION: boxer_runLoopShouldContinue called");
        logged = true;
    }

    // ... existing implementation ...
}

void boxer_prepareForFrameSize(int width, int height,
                                float aspect,  // OLD signature
                                uint8_t flags,
                                void* mode,
                                void* callback) {
    NSLog(@"❌ OLD INTEGRATION: boxer_prepareForFrameSize called with float aspect");

    // ... existing implementation ...
}
```

### Step 3.2: Run and Check Console

```bash
# Run Boxer with new scheme
# Watch Console.app or Xcode console

# Should see:
✅ NEW INTEGRATION: boxer_runLoopShouldContinue called
✅ NEW INTEGRATION: boxer_prepareForFrameSize called with Fraction aspect

# If you see ❌ OLD INTEGRATION messages, you're using the wrong version!
```

---

## Method 4: Debugger Verification

### Step 4.1: Set Breakpoint in New Integration

In Xcode:
1. Open `BXCoalface-New.mm`
2. Set breakpoint in `boxer_runLoopShouldContinue()`
3. Add breakpoint action: "Log message: Using NEW integration"

### Step 4.2: Set Breakpoint in Old Integration

1. Open `BXCoalface.mm` (old)
2. Set breakpoint in same function
3. Add breakpoint action: "Log message: Using OLD integration"

### Step 4.3: Run with Debugger

```bash
# Run Boxer with debugger attached
# When breakpoint hits, check:

1. Which file the breakpoint is in:
   - BXCoalface-New.mm ✅ Correct
   - BXCoalface.mm ❌ Wrong

2. Call stack shows correct DOSBox source:
   (lldb) bt
   # Should show paths containing "DOSBox-Staging-New"
```

---

## Method 5: Binary Verification

### Step 5.1: Check Linked Libraries

```bash
cd ~/boxer-upgrade/Boxer

# After building with new scheme
# Find the built app
find ~/Library/Developer/Xcode/DerivedData -name "Boxer.app" -type d | \
  grep "Debug-New-DOSBox" | head -1

# Save path
BOXER_NEW_APP="[path from above]"

# Check what's linked
otool -L "$BOXER_NEW_APP/Contents/MacOS/Boxer" | grep -i dosbox

# Should NOT show any separate DOSBox libraries
# (Since we compile everything together)

# Check symbols
nm "$BOXER_NEW_APP/Contents/MacOS/Boxer" | grep boxer_getDOSBoxIntegrationVersion

# Should show the function exists
```

### Step 5.2: Compare Binary Sizes

```bash
# Build both versions
xcodebuild -project Boxer.xcodeproj -scheme "Boxer" clean build
OLD_SIZE=$(du -h ~/Library/Developer/Xcode/DerivedData/*/Build/Products/Debug/Boxer.app/Contents/MacOS/Boxer | cut -f1)

xcodebuild -project Boxer.xcodeproj -scheme "Boxer (New DOSBox)" clean build
NEW_SIZE=$(du -h ~/Library/Developer/Xcode/DerivedData/*/Build/Products/Debug-New-DOSBox/Boxer.app/Contents/MacOS/Boxer | cut -f1)

echo "Old binary size: $OLD_SIZE"
echo "New binary size: $NEW_SIZE"

# New version should be noticeably larger (new DOSBox has more features)
# If sizes are identical, something is wrong
```

---

## Method 6: Feature-Based Verification

### Step 6.1: Test New DOSBox Features

The new DOSBox 0.83.0 has features the old 0.78.0 doesn't have:

**Test 1: Version String**

Add to DOSBox startup code in new version:

In `DOSBox-Staging-New/src/dosbox.cpp`:

```cpp
void DOSBOX_Init() {
    // ... existing init code ...

    LOG_MSG("DOSBox Staging version: %s", VERSION);
    LOG_MSG("Integration: NEW (0.83.0)");

    // ... rest of init ...
}
```

In old `DOSBox-Staging/src/dosbox.cpp`:

```cpp
void DOSBOX_Init() {
    // ... existing init code ...

    LOG_MSG("DOSBox Staging version: %s", VERSION);
    LOG_MSG("Integration: OLD (0.78.0)");

    // ... rest of init ...
}
```

**Test 2: Check for New Config Options**

New DOSBox has settings old one doesn't:

```cpp
// In new DOSBox startup, test for new features
if (control->GetSection("soundcanvas")) {
    LOG_MSG("✅ NEW DOSBox: SoundCanvas section found");
} else {
    LOG_MSG("❌ OLD DOSBox: SoundCanvas section NOT found");
}
```

**Test 3: File Path Test**

```objc
// In BXCoalface-New.mm
void boxer_shellWillStart() {
    // Try to access new file structure
    #include "audio/mixer.h"  // New path
    NSLog(@"✅ NEW: Using audio/mixer.h");

    // ... implementation ...
}
```

```objc
// In old BXCoalface.mm
void boxer_shellWillStart() {
    #include "mixer.h"  // Old path
    NSLog(@"❌ OLD: Using mixer.h");

    // ... implementation ...
}
```

---

## Method 7: Xcode Configuration Verification

### Step 7.1: Check Active Configuration

In Xcode:
1. Select your target (Boxer)
2. Editor → Add Build Setting → Add User-Defined Setting
3. Name: `INTEGRATION_TYPE`
4. For "Debug-New-DOSBox" config: Set value to `NEW`
5. For "Debug" config: Set value to `OLD`

Add to preprocessor:

```cpp
// In build settings, Preprocessor Macros
// For Debug-New-DOSBox:
DOSBOX_NEW_INTEGRATION=1
INTEGRATION_TYPE="NEW"

// For Debug:
DOSBOX_NEW_INTEGRATION=0
INTEGRATION_TYPE="OLD"
```

### Step 7.2: Display in Code

```objc
- (void)didFinishLaunching {
    NSLog(@"Build configuration integration type: %s", INTEGRATION_TYPE);

    #ifdef DOSBOX_NEW_INTEGRATION
    NSLog(@"DOSBOX_NEW_INTEGRATION is defined: %d", DOSBOX_NEW_INTEGRATION);
    #else
    NSLog(@"DOSBOX_NEW_INTEGRATION is NOT defined (using old)");
    #endif
}
```

---

## Quick Verification Checklist

When you build and run Boxer with the new scheme, verify ALL of these:

### Build Time:
- [ ] Xcode scheme shows "Boxer (New DOSBox)"
- [ ] Build log shows `DOSBox-Staging-New/` files being compiled
- [ ] Build log shows `BXCoalface-New.mm` being compiled
- [ ] Build log shows `-DDOSBOX_NEW_INTEGRATION=1`
- [ ] Build log shows header paths include `DOSBox-Staging-New/`

### Runtime:
- [ ] Console shows: "DOSBox Integration Version: NEW-0.83.0"
- [ ] Console shows: "✅ NEW INTEGRATION" callback logs
- [ ] Console does NOT show: "❌ OLD INTEGRATION" logs
- [ ] DOSBox version string is 0.83.0-alpha (not 0.78.0)

### Debugger:
- [ ] Breakpoints hit in `BXCoalface-New.mm` (not `BXCoalface.mm`)
- [ ] Call stack shows paths containing "DOSBox-Staging-New"
- [ ] Variables use new types (e.g., `Fraction` not `float`)

### Behavioral:
- [ ] New DOSBox features accessible (SoundCanvas config, etc.)
- [ ] Binary size different from old version
- [ ] Include paths work with new structure (audio/mixer.h)

**If ANY of these fail, you're not using the new integration!**

---

## Common Misconfiguration Causes

### Problem: Still Using Old Integration

**Possible causes:**

1. **Wrong Xcode Scheme Selected**
   - Check: Product → Scheme → should show "Boxer (New DOSBox)"
   - Fix: Select correct scheme

2. **Build Configuration Not Set**
   - Check: Scheme settings → Build Configuration
   - Should be: "Debug-New-DOSBox" not "Debug"
   - Fix: Edit scheme, change configuration

3. **Header Search Paths Wrong Order**
   - Check: Build Settings → Header Search Paths
   - Old paths listed before new paths
   - Fix: Reorder so `DOSBox-Staging-New/**` comes first

4. **Preprocessor Macro Not Defined**
   - Check: Build Settings → Preprocessor Macros
   - Should see: `DOSBOX_NEW_INTEGRATION=1`
   - Fix: Add to Debug-New-DOSBox configuration

5. **Linking Wrong Files**
   - Check: Build Phases → Compile Sources
   - Verify `BXCoalface-New.mm` is included
   - Verify DOSBox-Staging-New .cpp files are included
   - Fix: Add/remove files as needed

6. **Stale Build Cache**
   - Symptom: Changes not reflected in build
   - Fix: Product → Clean Build Folder (Shift+Cmd+K)
   - Then: Build again

7. **Both Integrations Compiled**
   - Check: Build log shows both old and new files
   - Problem: No configuration-specific compilation rules
   - Fix: Use conditional compilation or separate targets

---

## Emergency: "I Can't Tell Which Version I'm Running"

### Nuclear Option - Start Fresh

```bash
# 1. Clean everything
cd ~/boxer-upgrade/Boxer
rm -rf ~/Library/Developer/Xcode/DerivedData/*

# 2. Clean Xcode build
xcodebuild -project Boxer.xcodeproj -alltargets clean

# 3. Verify schemes exist
xcodebuild -project Boxer.xcodeproj -list

# Should see:
#   Schemes:
#     Boxer
#     Boxer (New DOSBox)

# 4. Build ONLY new version
xcodebuild -project Boxer.xcodeproj \
           -scheme "Boxer (New DOSBox)" \
           -configuration Debug-New-DOSBox \
           clean build

# 5. Find built app
find ~/Library/Developer/Xcode/DerivedData -name "Boxer.app" -type d

# 6. Run it
open [path-to-Boxer.app]

# 7. Check logs immediately
# Should see NEW integration messages
```

---

## Best Practice: Verification Script

Create a script to verify automatically:

```bash
#!/bin/bash
# verify-integration.sh

echo "=== Boxer DOSBox Integration Verification ==="
echo ""

# Check active scheme
SCHEME=$(xcodebuild -project Boxer.xcodeproj -showBuildSettings | grep -m1 "SCHEME_NAME" | awk '{print $3}')
echo "Active Scheme: $SCHEME"

# Expected: "Boxer (New DOSBox)"
if [[ "$SCHEME" == *"New DOSBox"* ]]; then
    echo "✅ Correct scheme"
else
    echo "❌ Wrong scheme - should contain 'New DOSBox'"
fi

# Check for compiled files
echo ""
echo "Checking last build..."

BUILD_DIR=$(find ~/Library/Developer/Xcode/DerivedData -name "Boxer.app" -type d | grep "Debug-New-DOSBox" | head -1)

if [ -z "$BUILD_DIR" ]; then
    echo "❌ No Debug-New-DOSBox build found"
    exit 1
else
    echo "✅ Found new DOSBox build: $BUILD_DIR"
fi

# Check binary for version function
echo ""
echo "Checking binary symbols..."

BINARY="$BUILD_DIR/Contents/MacOS/Boxer"
if nm "$BINARY" | grep -q "boxer_getDOSBoxIntegrationVersion"; then
    echo "✅ Version function found in binary"
else
    echo "❌ Version function NOT found"
fi

echo ""
echo "=== Run the app and check console for version messages ==="
```

Usage:
```bash
chmod +x verify-integration.sh
./verify-integration.sh
```

---

## Summary: How to Be Certain

**At Build Time:**
1. ✅ Check Xcode scheme selector (top bar)
2. ✅ Review build log for correct source paths
3. ✅ Verify preprocessor macros defined

**At Runtime:**
1. ✅ Check console for version string
2. ✅ Check console for callback logs
3. ✅ Verify DOSBox version number

**With Debugger:**
1. ✅ Set breakpoints in both old and new files
2. ✅ See which one hits
3. ✅ Inspect call stack

**If in doubt:**
- Clean everything
- Rebuild with explicit scheme selection
- Add logging to both old and new files
- See which logs appear

**The ultimate test:**
Add a deliberate syntax error to `BXCoalface.mm` (old). If your build still succeeds, you're using the new integration. If it fails, you're using the old one (or both).

---

**Document Version:** 1.0
**Date:** 2025-11-17
**Always verify before proceeding to next phase!**
