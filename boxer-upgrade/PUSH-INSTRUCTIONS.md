# How to Push Phase 1 DOSBox Staging Changes

## Summary

Phase 1 made 5 commits to the `dosbox-staging` repository that need to be pushed to the remote `eduo/dosbox-staging` repository on branch `dosbox-boxer-upgrade-dosboxside`.

Due to Claude Code Web's git proxy limitations, these commits cannot be pushed directly from the web environment. You'll need to apply them from your local machine.

---

## How to Apply the Patch File

A patch file containing all 5 commits has been created: `phase-1-dosbox-changes.patch`

This is pushed to the main `dosbox-staging-boxer` repository, so you can download it from there.

### Steps:

1. **On your local machine**, clone or navigate to your dosbox-staging repository:
   ```bash
   cd /path/to/your/dosbox-staging

   # Or if you need to clone it first:
   # git clone https://github.com/eduo/dosbox-staging.git
   # cd dosbox-staging
   ```

2. **Checkout the correct branch**:
   ```bash
   git checkout dosbox-boxer-upgrade-dosboxside

   # If the branch doesn't exist locally, create it:
   # git checkout -b dosbox-boxer-upgrade-dosboxside origin/dosbox-boxer-upgrade-dosboxside
   ```

3. **Download the patch file from the main repository**:

   **Option A - If you have dosbox-staging-boxer cloned locally**:
   ```bash
   # Navigate to your local dosbox-staging-boxer repo
   cd /path/to/dosbox-staging-boxer
   git checkout claude/phase-1-orchestrator-01H1B4jhqBRczSnhhSghWmN6
   git pull

   # Copy the patch to your dosbox-staging repo
   cp boxer-upgrade/phase-1-dosbox-changes.patch /path/to/dosbox-staging/
   cd /path/to/dosbox-staging
   ```

   **Option B - Download directly from GitHub**:
   ```bash
   cd /path/to/dosbox-staging
   curl -O https://raw.githubusercontent.com/eduo/dosbox-staging-boxer/claude/phase-1-orchestrator-01H1B4jhqBRczSnhhSghWmN6/boxer-upgrade/phase-1-dosbox-changes.patch
   ```

4. **Apply the patch**:
   ```bash
   git am < phase-1-dosbox-changes.patch
   ```

   If you get errors about whitespace or conflicts:
   ```bash
   # Try with 3-way merge
   git am -3 < phase-1-dosbox-changes.patch

   # If that fails, you can skip whitespace checks:
   git am --whitespace=fix < phase-1-dosbox-changes.patch
   ```

5. **Verify the commits were applied**:
   ```bash
   git log --oneline -5
   ```

   You should see:
   ```
   8cf253c Add INT-059 emergency abort hook to main emulation loop
   a162171 TASK 1-4: Add Boxer source integration to CMake build
   670858d Phase 1 - TASK 1-3: Create stub implementation
   35d13c9 Phase 1: Add Boxer hook infrastructure headers
   590fac3 Add BOXER_INTEGRATED CMake option for library build mode
   ```

6. **Push to remote**:
   ```bash
   git push origin dosbox-boxer-upgrade-dosboxside
   ```

---

## Why Not Direct Access?

**Note**: The web session's file system (`/home/user/dosbox-staging-boxer/...`) is not accessible from your local machine because:
- Claude Code Web runs in an isolated container
- The file system is not network-mounted or remotely accessible
- Git remotes require either network protocols (http/https/git/ssh) or local filesystem access

This is why the **patch file is the only option** for transferring these commits from the web session to your local environment.

---

## The 5 Commits to Push

All commits are on branch `dosbox-boxer-upgrade-dosboxside` in the web session:

### 1. `590fac339` - Add BOXER_INTEGRATED CMake option for library build mode
- **File**: `CMakeLists.txt`
- **Changes**: +42/-6 lines
- **Purpose**: Enables building DOSBox as static library

### 2. `35d13c951` - Phase 1: Add Boxer hook infrastructure headers
- **Files**:
  - `include/boxer/boxer_hooks.h` (1006 lines, new)
  - `include/boxer/boxer_types.h` (89 lines, new)
- **Purpose**: Defines IBoxerDelegate interface with 86 integration points

### 3. `670858d94` - Phase 1 - TASK 1-3: Create stub implementation for boxer_hooks.cpp
- **File**: `src/boxer/boxer_hooks.cpp` (18 lines, new)
- **Purpose**: Defines global delegate pointer

### 4. `a162171f2` - TASK 1-4: Add Boxer source integration to CMake build
- **File**: `CMakeLists.txt`
- **Changes**: Adds boxer_hooks.cpp to build, includes boxer headers
- **Purpose**: Integrates Boxer sources into CMake build system

### 5. `8cf253c62` - Add INT-059 emergency abort hook to main emulation loop
- **File**: `src/dosbox.cpp`
- **Changes**: +11 lines
- **Purpose**: Critical emergency abort mechanism in emulation loop

---

## Verification

After pushing, verify all commits are present:

```bash
git log --oneline dosbox-boxer-upgrade-dosboxside -10
```

You should see all 5 commits at the top of the log.

---

## Current Status

- ✅ All 5 commits created locally in web session
- ✅ Patch file generated and committed to main repository
- ⏳ **PENDING**: Push to `eduo/dosbox-staging` remote
- Location: Web session at `/home/user/dosbox-staging-boxer/boxer-upgrade/src/dosbox-staging`

---

## Files Modified in DOSBox Staging

```
CMakeLists.txt                          (modified: +42/-6 + integration)
include/boxer/boxer_hooks.h             (new: 1006 lines)
include/boxer/boxer_types.h             (new: 89 lines)
src/boxer/boxer_hooks.cpp               (new: 18 lines)
src/dosbox.cpp                          (modified: +11 lines)
```

**Total**: 5 files, ~1,166 lines added

---

## Questions?

If you encounter issues applying the patch:
1. Check that you're on the correct branch (`dosbox-boxer-upgrade-dosboxside`)
2. Ensure your working directory is clean (`git status`)
3. Try `git am --abort` if a previous attempt failed, then retry
4. Check patch format: `git apply --check phase-1-dosbox-changes.patch`
