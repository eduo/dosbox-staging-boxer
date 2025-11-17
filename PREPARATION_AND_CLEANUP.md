# Boxer Preparation and Cleanup Guide

**Version:** 1.0
**Date:** 2025-11-17
**Purpose:** Prepare Boxer codebase for DOSBox Staging upgrade and handle existing integration

## Critical Issue Identified

The original migration guide (Phase 3) instructs copying BXCoalface files into new DOSBox **without first addressing the existing integration**. This will cause conflicts because:

1. Boxer currently references the OLD DOSBox Staging (submodule or vendored code)
2. BXCoalface files in Boxer are tailored to OLD DOSBox API (0.78.0)
3. Boxer's Xcode project compiles against OLD DOSBox source files
4. There's no clear transition path from old→new

**This document provides the missing preparation steps that should occur BEFORE Phase 3.**

---

## Phase 0: Backup and Environment Preparation (Week 0)

### Objective
Create a safe working environment with backups, branches, and parallel builds.

### Step 0.1: Create Complete Backup

```bash
# Create backup directory
mkdir -p ~/boxer-upgrade-backup-$(date +%Y%m%d)
BACKUP_DIR=~/boxer-upgrade-backup-$(date +%Y%m%d)

# Backup current working Boxer
cd ~/boxer-upgrade/Boxer
tar -czf $BACKUP_DIR/boxer-before-upgrade.tar.gz .

# Backup old DOSBox integration
cd ~/boxer-upgrade/dosbox-staging-old
tar -czf $BACKUP_DIR/dosbox-old-integration.tar.gz .

# Document current versions
cd ~/boxer-upgrade/Boxer
git log -1 --format="Boxer: %H %s" > $BACKUP_DIR/version-info.txt
git describe --always --tags >> $BACKUP_DIR/version-info.txt

cd DOSBox-Staging  # or wherever the submodule is
git log -1 --format="DOSBox: %H %s" >> $BACKUP_DIR/version-info.txt

# Backup successful Boxer build (if available)
if [ -d ~/Library/Developer/Xcode/DerivedData ]; then
    # Find Boxer build products
    find ~/Library/Developer/Xcode/DerivedData -name "Boxer.app" -exec cp -R {} $BACKUP_DIR/ \;
fi

echo "Backup created at: $BACKUP_DIR"
ls -lh $BACKUP_DIR/
```

**Checkpoint:**
- [ ] Backups created successfully
- [ ] Version information documented
- [ ] Working Boxer.app backed up (if available)

### Step 0.2: Verify Current Boxer Builds

```bash
cd ~/boxer-upgrade/Boxer

# Try building current version
xcodebuild -project Boxer.xcodeproj \
           -scheme Boxer \
           -configuration Debug \
           clean build \
           CONFIGURATION_BUILD_DIR=$PWD/build-old 2>&1 | tee build-old.log

# Check if successful
if [ -d build-old/Boxer.app ]; then
    echo "✅ Current Boxer builds successfully"

    # Test run
    open build-old/Boxer.app
    # Manually verify it works, then close

else
    echo "❌ Current Boxer does not build"
    echo "STOP: Fix current build before proceeding"
    exit 1
fi
```

**Checkpoint:**
- [ ] Current Boxer builds without errors
- [ ] Current Boxer runs and functions
- [ ] Build log saved for reference

### Step 0.3: Create Feature Branch in Boxer

```bash
cd ~/boxer-upgrade/Boxer

# Create feature branch for upgrade work
git checkout -b feature/dosbox-staging-0.83-upgrade

# Tag the starting point
git tag -a boxer-pre-dosbox-upgrade -m "Boxer state before DOSBox Staging 0.83.0 upgrade"

# Push branch and tag
git push -u origin feature/dosbox-staging-0.83-upgrade
git push origin boxer-pre-dosbox-upgrade
```

**Checkpoint:**
- [ ] Feature branch created
- [ ] Starting point tagged
- [ ] Can revert to this point if needed

---

## Phase 0.5: Analyze Current Boxer-DOSBox Integration (Week 0, Day 2)

### Objective
Understand exactly how Boxer currently integrates DOSBox.

### Step 0.5.1: Identify Integration Method

```bash
cd ~/boxer-upgrade/Boxer

# Check for DOSBox submodule
git submodule status

# Check for vendored DOSBox code
find . -name "dosbox.cpp" -o -name "DOSBOX*" -type d

# Check Xcode project for DOSBox references
grep -r "dosbox" Boxer.xcodeproj/project.pbxproj | head -20

# Check for DOSBox source files in Xcode
# (This will vary based on how Boxer is structured)
```

**Common patterns:**

**Pattern A: Git Submodule**
```bash
# If submodule exists at DOSBox-Staging/
cd DOSBox-Staging
git remote -v
# Shows: https://github.com/MaddTheSane/dosbox-staging.git
```

**Pattern B: Vendored Code**
```bash
# DOSBox source copied directly into Boxer repo
# No .git in DOSBox directory
ls -la DOSBox-Staging/.git  # Would not exist
```

**Pattern C: External Dependency**
```bash
# DOSBox built separately, linked as library
# Check for .dylib or .framework
find . -name "*.dylib" | grep -i dosbox
find . -name "*.framework" | grep -i dosbox
```

**Document your finding:**
```bash
echo "Integration method: [SUBMODULE/VENDORED/EXTERNAL]" > $BACKUP_DIR/integration-method.txt
```

### Step 0.5.2: Inventory BXCoalface Files

```bash
cd ~/boxer-upgrade/Boxer

# Find all Boxer integration files
find . -name "BX*.h" -o -name "BX*.mm" -o -name "BX*.m" | sort > $BACKUP_DIR/bxcoalface-files.txt

# For each file, note its purpose
cat $BACKUP_DIR/bxcoalface-files.txt

# Typical files:
# ./Boxer/BXCoalface.h          - Main C++ callbacks
# ./Boxer/BXCoalface.mm         - Objective-C++ implementation
# ./Boxer/BXCoalfaceAudio.h     - Audio callbacks
# ./Boxer/BXCoalfaceAudio.mm    - Audio implementation
# ./Boxer/BXEmulator.h          - Main emulator controller
# ./Boxer/BXEmulator.mm         - Implementation
```

### Step 0.5.3: Identify DOSBox Source Files in Xcode

```bash
cd ~/boxer-upgrade/Boxer

# Extract DOSBox source files from Xcode project
# This is complex - easier to open in Xcode GUI

# OR use PBXProj tools
# This shows all .cpp files in the project
grep "\.cpp" Boxer.xcodeproj/project.pbxproj | \
    grep -v "\.m" | \
    grep -v "BX" | \
    sort > $BACKUP_DIR/dosbox-sources-in-xcode.txt

# Review the list
head -20 $BACKUP_DIR/dosbox-sources-in-xcode.txt
```

**Checkpoint:**
- [ ] Integration method identified
- [ ] BXCoalface files inventoried
- [ ] DOSBox source files in Xcode documented

---

## Phase 0.75: Clean Separation Strategy (Week 0, Day 3)

### Objective
Decide on the transition approach.

### Option A: In-Place Upgrade (Higher Risk)

**Approach:**
- Update DOSBox submodule/code in place
- Update BXCoalface files to match new API
- Fix compilation errors incrementally

**Pros:**
- Single codebase
- Simpler repo structure

**Cons:**
- Can't easily revert
- No side-by-side testing
- Higher risk

### Option B: Parallel Development (Recommended)

**Approach:**
- Keep old DOSBox integration intact
- Create new DOSBox integration alongside
- Use Xcode build configurations to switch between them
- Gradual migration with safety net

**Pros:**
- Can always revert to working version
- Side-by-side comparison
- Incremental migration
- Lower risk

**Cons:**
- More complex initially
- Larger repo temporarily
- Need to maintain both during transition

**Recommendation: Option B (Parallel)**

---

## Phase 1-prep: Parallel Integration Setup (Week 1, Days 1-2)

### Objective
Set up new DOSBox alongside old, with ability to build both.

### Step 1.1: Create New DOSBox Directory

```bash
cd ~/boxer-upgrade/Boxer

# If using submodule, add new one
git submodule add -b dosbox-boxer-upgrade-dosboxside \
    https://github.com/eduo/dosbox-staging.git \
    DOSBox-Staging-New

# If vendored, create new directory
mkdir DOSBox-Staging-New
cd DOSBox-Staging-New
git clone -b dosbox-boxer-upgrade-dosboxside \
    https://github.com/eduo/dosbox-staging.git .
cd ..

# Now you have both:
# DOSBox-Staging/       - Old (0.78.0)
# DOSBox-Staging-New/   - New (0.83.0)
```

### Step 1.2: Create New BXCoalface Files

```bash
cd ~/boxer-upgrade/Boxer/Boxer

# Copy existing files as templates for new version
cp BXCoalface.h BXCoalface-New.h
cp BXCoalface.mm BXCoalface-New.mm
cp BXCoalfaceAudio.h BXCoalfaceAudio-New.h
cp BXCoalfaceAudio.mm BXCoalfaceAudio-New.mm

# These will be updated for new DOSBox API
```

### Step 1.3: Update New BXCoalface Files for New Paths

```bash
cd ~/boxer-upgrade/Boxer/Boxer

# Edit BXCoalface-New.h
# Update all includes:
```

Edit `BXCoalface-New.h`:
```cpp
// OLD includes (for 0.78.0)
#include "mixer.h"
#include "render.h"
#include "keyboard.h"

// NEW includes (for 0.83.0)
#include "audio/mixer.h"
#include "gui/render/render.h"
#include "hardware/input/keyboard.h"
// ... etc (see INTEGRATION_MAPPING.md)
```

Update callback signatures:
```cpp
// OLD signature
void boxer_prepareForFrameSize(int width, int height, float aspect,
                                uint8_t flags, void* mode, void* callback);

// NEW signature
uint8_t boxer_prepareForFrameSize(int width, int height,
                                    Fraction aspect_ratio,
                                    uint8_t flags,
                                    VideoMode& mode,
                                    GFX_Callback_t callback);

// Add helper
inline float FractionToFloat(Fraction f) {
    return static_cast<float>(f.numerator) / static_cast<float>(f.denominator);
}
```

### Step 1.4: Create Xcode Build Configurations

Open `Boxer.xcodeproj` in Xcode:

1. **Duplicate Debug configuration:**
   - Project Settings → Info → Configurations
   - Duplicate "Debug" → "Debug-New-DOSBox"
   - Duplicate "Release" → "Release-New-DOSBox"

2. **Create preprocessor macros:**
   - Select "Debug-New-DOSBox" configuration
   - Build Settings → Preprocessor Macros
   - Add: `DOSBOX_NEW_INTEGRATION=1`

3. **Update header search paths for new config:**
   - Build Settings → Header Search Paths
   - For "Debug-New-DOSBox":
     - Remove: `$(SRCROOT)/DOSBox-Staging/**`
     - Add: `$(SRCROOT)/DOSBox-Staging-New/**`

4. **Create scheme for new DOSBox:**
   - Product → Scheme → New Scheme
   - Name: "Boxer (New DOSBox)"
   - Set to use "Debug-New-DOSBox" configuration

### Step 1.5: Update Xcode Project Source References

**Critical:** Need to tell Xcode to compile NEW DOSBox sources for new configuration.

In Xcode:
1. Select all DOSBox source files in project navigator
2. Right-click → Get Info
3. Look for "Target Membership" or similar
4. May need to create separate targets or use configurations

**Alternative approach:**
Create a new Xcode target for new DOSBox:

1. File → New → Target
2. Name: "DOSBox-New"
3. Type: Static Library
4. Add DOSBox-Staging-New source files to this target
5. Link Boxer against this new library in new configuration

### Step 1.6: Conditional Compilation in BXEmulator

Edit `BXEmulator.h`:
```objc
#ifdef DOSBOX_NEW_INTEGRATION
#import "BXCoalface-New.h"
#else
#import "BXCoalface.h"
#endif

@interface BXEmulator : NSObject

// Rendering callbacks - signature changed in new version
#ifdef DOSBOX_NEW_INTEGRATION
- (uint8_t)_prepareForFrameWidth:(int)width
                          height:(int)height
                     aspectRatio:(float)aspect  // Will convert from Fraction
                           flags:(uint8_t)flags
                       videoMode:(void*)mode
                        callback:(void*)callback;

- (BOOL)_startFrameWithPixels:(uint8_t**)pixels pitch:(int*)pitch;
#else
- (uint8_t)_prepareForFrameWidth:(int)width
                          height:(int)height
                     aspectRatio:(float)aspect
                           flags:(uint8_t)flags;

- (void)_startFrame;
#endif

// ... other methods ...

@end
```

Edit `BXCoalface-New.mm`:
```objc
#ifdef DOSBOX_NEW_INTEGRATION

uint8_t boxer_prepareForFrameSize(int width, int height,
                                    Fraction aspect_ratio,
                                    uint8_t flags,
                                    VideoMode& mode,
                                    GFX_Callback_t callback) {
    float aspect_float = FractionToFloat(aspect_ratio);

    BXEmulator *emulator = [BXEmulator currentEmulator];
    return [emulator _prepareForFrameWidth:width
                                    height:height
                               aspectRatio:aspect_float
                                     flags:flags
                                 videoMode:&mode
                                  callback:callback];
}

BOOL boxer_startFrame(uint8_t** pixels_out, int* pitch_out) {
    BXEmulator *emulator = [BXEmulator currentEmulator];
    return [emulator _startFrameWithPixels:pixels_out pitch:pitch_out];
}

// ... other new callbacks ...

#endif // DOSBOX_NEW_INTEGRATION
```

**Checkpoint:**
- [ ] New DOSBox directory created
- [ ] New BXCoalface files created with updated signatures
- [ ] Xcode configurations created
- [ ] Can switch between old and new via scheme selection
- [ ] Old version still builds and runs

---

## Phase 2-prep: Copy Integration Files to New DOSBox (Week 1, Day 3)

**NOW we can safely do what Phase 3 originally described.**

### Step 2.1: Copy BXCoalface to New DOSBox

```bash
cd ~/boxer-upgrade/Boxer

# Copy NEW versions to new DOSBox
cp Boxer/BXCoalface-New.h DOSBox-Staging-New/src/BXCoalface.h
cp Boxer/BXCoalface-New.mm DOSBox-Staging-New/src/BXCoalface.mm
cp Boxer/BXCoalfaceAudio-New.h DOSBox-Staging-New/src/BXCoalfaceAudio.h
cp Boxer/BXCoalfaceAudio-New.mm DOSBox-Staging-New/src/BXCoalfaceAudio.mm

# Verify
ls -l DOSBox-Staging-New/src/BX*
```

### Step 2.2: Inject into New DOSBox

```bash
cd ~/boxer-upgrade/Boxer/DOSBox-Staging-New

# Edit src/dosbox.h
# Add at end:
```

```cpp
// At end of src/dosbox.h
#ifdef BOXER_INTEGRATION
#include "BXCoalface.h"
#endif
```

### Step 2.3: Update New DOSBox Build System

Follow the original Phase 3 instructions, but NOW:
- Old DOSBox remains untouched
- New DOSBox gets the integration
- Both can be built via Xcode configurations

**Checkpoint:**
- [ ] BXCoalface files in new DOSBox
- [ ] dosbox.h updated
- [ ] Build system configured
- [ ] Old version still works
- [ ] New version ready for development

---

## Transition Workflow

### During Development

```bash
# Work on new integration
cd ~/boxer-upgrade/Boxer

# Open Xcode
open Boxer.xcodeproj

# Select scheme: "Boxer (New DOSBox)"
# Build and test new version

# If issues arise, switch back:
# Select scheme: "Boxer" (original)
# Verify old version still works
```

### Testing Protocol

1. **After each change to new integration:**
   - Build with "Boxer (New DOSBox)" scheme
   - Test specific feature
   - Document issues

2. **Periodically verify old version:**
   - Build with "Boxer" scheme
   - Quick smoke test
   - Ensures old version not broken

3. **Side-by-side comparison:**
   - Build both versions
   - Run same DOS program in both
   - Compare behavior

### When to Remove Old Integration

**Only after:**
- [ ] All features working in new version
- [ ] All tests passing
- [ ] Performance equivalent
- [ ] User acceptance testing complete
- [ ] Team approves transition

**Then:**
```bash
cd ~/boxer-upgrade/Boxer

# Remove old DOSBox
git rm -r DOSBox-Staging
# OR if submodule:
git submodule deinit DOSBox-Staging
git rm DOSBox-Staging

# Rename new to main
git mv DOSBox-Staging-New DOSBox-Staging

# Remove -New suffix from BXCoalface files
cd Boxer
git mv BXCoalface-New.h BXCoalface.h
git mv BXCoalface-New.mm BXCoalface.mm
# ... etc

# Update Xcode project
# Remove old configurations
# Make new configuration the default

# Commit
git add -A
git commit -m "Complete migration to DOSBox Staging 0.83.0"
```

---

## Rollback Procedure

If critical issues found:

### Emergency Rollback

```bash
cd ~/boxer-upgrade/Boxer

# Return to pre-upgrade tag
git checkout boxer-pre-dosbox-upgrade

# Verify
xcodebuild -project Boxer.xcodeproj -scheme Boxer clean build

# If working, create recovery branch
git checkout -b recovery-from-upgrade-issues
```

### Partial Rollback

```bash
# Just switch Xcode scheme back to old DOSBox
# No code changes needed if parallel setup used
```

---

## Summary: Corrected Migration Sequence

### Week 0: Preparation (THIS DOCUMENT)
- [ ] Phase 0: Backup and environment prep
- [ ] Phase 0.5: Analyze current integration
- [ ] Phase 0.75: Choose transition strategy
- [ ] Phase 1-prep: Set up parallel integration
- [ ] Phase 2-prep: Copy files to new DOSBox

### Weeks 1-2: Foundation (MIGRATION_GUIDE.md Phase 1)
- [ ] Update include paths in BXCoalface-New
- [ ] Update callback signatures
- [ ] Baseline build of new DOSBox

### Weeks 3-5: Core Integration (MIGRATION_GUIDE.md Phases 2-3)
- [ ] Implement hooks in new DOSBox
- [ ] Port rendering backend
- [ ] Port shell integration

### Weeks 6-7: Subsystems (MIGRATION_GUIDE.md Phase 4)
- [ ] Audio/MIDI
- [ ] Parallel port decision
- [ ] Testing

### Weeks 8-10: Finalization (MIGRATION_GUIDE.md Phases 5-6)
- [ ] Comprehensive testing
- [ ] Performance validation
- [ ] Remove old integration
- [ ] Documentation

---

## Critical Success Factors

1. **Always maintain a working version** - Parallel setup ensures this
2. **Test frequently** - Switch between old and new often
3. **Document issues** - Keep log of problems and solutions
4. **Incremental migration** - Don't try to port everything at once
5. **Team communication** - Regular status updates

---

**Document Version:** 1.0
**Date:** 2025-11-17
**Status:** **MANDATORY - Do this BEFORE original Phase 3**
