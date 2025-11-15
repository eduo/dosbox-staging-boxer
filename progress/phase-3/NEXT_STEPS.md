# Phase 3: Next Steps for Human

**Summary**: DOSBox library is built ✅ | Need 15-30 min of Xcode work to complete

---

## Quick Start (What You Need To Do)

### Step 1: Open Project
```bash
cd /Users/eduo/Developer/dosbox-staging-boxer/boxer-upgrade/src/boxer
open Boxer.xcodeproj
```

### Step 2: Add Libraries (2 minutes)
1. Select **Boxer** project (top of navigator)
2. Select **Boxer** target (not Bundler)
3. **Build Phases** tab
4. **Link Binary With Libraries** → **+** button
5. **Add Other...** → **Add Files...**
6. Navigate to: `../dosbox-staging/build-boxer/`
7. Select **libdosbox.a** → **Open**
8. Repeat to add **liblibdosboxcommon.a**

### Step 3: Add Header Paths (1 minute)
1. **Build Settings** tab
2. Search: `Header Search Paths`
3. Double-click value
4. **+** button, add: `$(SRCROOT)/../dosbox-staging/include` (recursive ✓)
5. **+** button, add: `$(SRCROOT)/../dosbox-staging/src` (recursive ✓)

### Step 4: Add Preprocessor Macro (1 minute)
1. Still in **Build Settings**
2. Search: `Preprocessor Macros`
3. **Debug** → double-click → **+** → add: `BOXER_INTEGRATED=1`
4. **Release** → double-click → **+** → add: `BOXER_INTEGRATED=1`

### Step 5: Add BoxerDelegate Files (1 minute)
1. In **Project Navigator** (left panel)
2. Find **Boxer** folder
3. Find the two new files:
   - `BXEmulator+BoxerDelegate.h`
   - `BXEmulator+BoxerDelegate.mm`
4. If they have grayed-out names (not in target):
   - Click each file
   - **File Inspector** (right panel)
   - **Target Membership** → check **Boxer**

### Step 6: Initialize Delegate (5 minutes)
1. Open `Boxer/BXEmulator.mm`
2. **Add at top** (after other imports, around line 15):
```objc
#ifdef BOXER_INTEGRATED
#import "BXEmulator+BoxerDelegate.h"
extern "C" {
    extern void* g_boxer_delegate;
}
#endif
```

3. **Find the `- (void) start` method** (around line 235)
4. **Add before DOSBox initialization** (after the `isCancelled` check):
```objc
#ifdef BOXER_INTEGRATED
    // Set global delegate pointer for DOSBox hooks
    g_boxer_delegate = (__bridge void*)self;
#endif
```

5. **Find where emulator stops/cleans up** (search for `dealloc` or similar)
6. **Add delegate cleanup**:
```objc
#ifdef BOXER_INTEGRATED
    g_boxer_delegate = nullptr;
#endif
```

7. **Save** (Cmd+S)

### Step 7: Build (1 minute)
1. Select scheme: **Boxer** → **My Mac**
2. **Product** → **Build** (or Cmd+B)
3. Wait for build to complete

**Expected**: Build should succeed with no errors

**If errors**, see "Common Errors" section in `COMPILATION_STATUS.md`

### Step 8: Test (5 minutes)
1. **Product** → **Run** (or Cmd+R)
2. Boxer should launch
3. Create/open a gamebox
4. Boot to DOS prompt
5. **Check**:
   - DOS prompt visible? ✓
   - Can type commands? ✓
   - Colors correct? ✓

**If it works**: Phase 3 is successfully integrated! ✨

---

## Common Errors

### Error: 'boxer/boxer_hooks.h' file not found
→ Check Step 3 (Header Search Paths)

### Error: Undefined symbol: _g_boxer_delegate  
→ Check Step 2 (Libraries linked)

### Error: Use of undeclared identifier 'IBoxerDelegate'
→ Check Step 6 (Import BoxerDelegate.h)

---

## Verification

After successful build, verify Phase 3 hooks are active:

1. Open **Console.app**
2. Filter for: `Boxer`
3. In `BXEmulator+BoxerDelegate.mm`, add:
```objc
- (bool) startFrame: (uint8_t**)pixels_out pitch: (int*)pitch_out {
    NSLog(@"🎨 Phase 3 startFrame hook called!");
    // ... rest of method
}
```
4. Rebuild and run
5. Boot DOS - you should see logs in Console!

---

## If You Get Stuck

1. Check `COMPILATION_STATUS.md` for detailed troubleshooting
2. Check `TESTING_GUIDE.md` for comprehensive testing steps  
3. All Phase 3 documentation is in `progress/phase-3/`

---

## Summary

**Time Required**: 15-30 minutes  
**Difficulty**: Easy (following steps)  
**Result**: Boxer with modern DOSBox Staging integrated! 🚀

---

**You're almost there!** The hard work (coding) is done. Just need to wire it up in Xcode.

Good luck! 🎮
