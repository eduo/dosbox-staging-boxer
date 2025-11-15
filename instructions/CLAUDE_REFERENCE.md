# Claude Instance Reference Guide

## Quick Start for New Claude Instances

This document explains how to work with this multi-repository project structure.

### ⚠️ BEFORE YOU START: Verify Repositories

**First action when starting any session:**

```bash
ls -la /home/user/dosbox-staging-boxer/src/
```

If you don't see `dosbox-staging/`, `boxer/`, and `dosbox-staging-legacy/` directories, **scroll down to "FIRST STEP: Verify Repository Setup"** and clone them.

This is **BLOCKER-001** - the project cannot proceed without these repositories.

---

## Directory Structure

```
/home/user/dosbox-staging-boxer/
    ├── instructions/                 ← Orchestrator guidance
    ├── progress/                     ← Phase tracking
    ├── validation/                   ← Smoke tests
    └── src/
        ├── dosbox-staging/           ← SEPARATE git repo
        │   └── (DOSBox Staging fork)
        ├── boxer/                    ← SEPARATE git repo
        │   └── (Boxer fork)
        └── dosbox-staging-legacy/    ← SEPARATE git repo (reference)
            └── (Legacy DOSBox for parport code)
```

**IMPORTANT**: All paths in this project are relative to `/home/user/dosbox-staging-boxer/`

---

## How to Access Files

### Reading Files
Use absolute paths with the Read tool:

```
Read: /home/user/dosbox-staging-boxer/src/dosbox-staging/CMakeLists.txt
Read: /home/user/dosbox-staging-boxer/src/boxer/BXEmulator.mm
```

### Editing Files
Use absolute paths with Edit/Write tools:

```
Edit: /home/user/dosbox-staging-boxer/src/dosbox-staging/include/boxer/boxer_hooks.h
Write: /home/user/dosbox-staging-boxer/validation/smoke-test/test.cpp
```

### Searching Files
Use Glob/Grep with paths:

```bash
Glob: pattern="**/boxer_hooks.h" path="/home/user/dosbox-staging-boxer/src/dosbox-staging"
Grep: pattern="IBoxerDelegate" path="/home/user/dosbox-staging-boxer/src"
```

---

## Git Operations

### Three Separate Repositories

1. **Main Repo** (dosbox-staging-boxer):
   - Path: `/home/user/dosbox-staging-boxer/`
   - Branch: `claude/phase-1-orchestrator-01H1B4jhqBRczSnhhSghWmN6`
   - Purpose: Project coordination, instructions, progress tracking

2. **DOSBox Staging Repo**:
   - Path: `/home/user/dosbox-staging-boxer/src/dosbox-staging/`
   - Branch: `dosbox-boxer-upgrade-dosboxside`
   - Purpose: DOSBox modifications (hooks, build system)

3. **Boxer Repo**:
   - Path: `/home/user/dosbox-staging-boxer/src/boxer/`
   - Branch: `boxer-dosbox-upgrade-boxerside`
   - Purpose: Boxer modifications (delegate implementation, UI)

### Making Commits

**For DOSBox Staging changes:**
```bash
cd /home/user/dosbox-staging-boxer/src/dosbox-staging
git status
git add include/boxer/boxer_hooks.h src/boxer/boxer_hooks.cpp
git commit -m "Add hook infrastructure"
```

**For Boxer changes:**
```bash
cd /home/user/dosbox-staging-boxer/src/boxer
git status
git add Boxer/BXEmulator.mm
git commit -m "Implement lifecycle hooks"
```

**For main repo changes:**
```bash
cd /home/user/dosbox-staging-boxer
git status
git add progress/phase-1/OBJECTIVES.md
git commit -m "Update Phase 1 objectives"
```

### Pushing Changes

**⚠️ CRITICAL LIMITATION**: Claude Code Web's git proxy **ONLY works for the main repository**.

- ✅ **Can push**: `/home/user/dosbox-staging-boxer/` (main repo)
- ❌ **Cannot push**: `src/dosbox-staging/` or `src/boxer/` subdirectories

**Solution**: Use patch files at the end of each phase.

---

## Phase Completion Workflow

At the end of each phase, create patch files for each repository that was modified.

### 1. Check DOSBox Staging Changes
```bash
cd /home/user/dosbox-staging-boxer/src/dosbox-staging
git log --oneline -10
git status
```

If there are commits, create patch file:
```bash
git format-patch dosbox-boxer-upgrade-dosboxside --stdout > ../../phase-N-dosbox-changes.patch
```

### 2. Check Boxer Changes
```bash
cd /home/user/dosbox-staging-boxer/src/boxer
git log --oneline -10
git status
```

If there are commits, create patch file:
```bash
git format-patch boxer-dosbox-upgrade-boxerside --stdout > ../../phase-N-boxer-changes.patch
```

### 3. Update PUSH-INSTRUCTIONS.md
Add instructions for applying the new patch files.

### 4. Push Main Repo Changes
```bash
cd /home/user/dosbox-staging-boxer
git commit -m "Complete Phase N"
git push -u origin claude/phase-1-orchestrator-01H1B4jhqBRczSnhhSghWmN6
```

---

## Verification Commands

### Check Repository Status
```bash
# Main repo
cd /home/user/dosbox-staging-boxer && git status

# DOSBox Staging
cd /home/user/dosbox-staging-boxer/src/dosbox-staging && git status

# Boxer
cd /home/user/dosbox-staging-boxer/src/boxer && git status
```

### Verify Branches
```bash
cd /home/user/dosbox-staging-boxer/src/dosbox-staging
git branch -v
# Should show: dosbox-boxer-upgrade-dosboxside

cd /home/user/dosbox-staging-boxer/src/boxer
git branch -v
# Should show: boxer-dosbox-upgrade-boxerside
```

### View Recent Commits
```bash
cd /home/user/dosbox-staging-boxer/src/dosbox-staging
git log --oneline -5

cd /home/user/dosbox-staging-boxer/src/boxer
git log --oneline -5
```

---

## Common Mistakes to Avoid

### ❌ Wrong: Using relative paths
```bash
Read: src/dosbox-staging/CMakeLists.txt  # Will fail
```

### ✅ Correct: Using absolute paths
```bash
Read: /home/user/dosbox-staging-boxer/src/dosbox-staging/CMakeLists.txt
```

### ❌ Wrong: Git commands without CD
```bash
Bash: git status src/dosbox-staging  # Wrong repo context
```

### ✅ Correct: CD first, then git
```bash
Bash: cd /home/user/dosbox-staging-boxer/src/dosbox-staging && git status
```

### ❌ Wrong: Trying to push subdirectories
```bash
cd /home/user/dosbox-staging-boxer/src/dosbox-staging
git push  # Will fail - authentication error
```

### ✅ Correct: Create patch file instead
```bash
cd /home/user/dosbox-staging-boxer/src/dosbox-staging
git format-patch dosbox-boxer-upgrade-dosboxside --stdout > ../../phase-1-dosbox-changes.patch
```

---

## Phase-Specific Repository Usage

| Phase | DOSBox Staging | Boxer | Main Repo |
|-------|----------------|-------|-----------|
| 1: Foundation | ✅ (hooks, build) | ❌ | ✅ (tracking) |
| 2: Lifecycle | ✅ (lifecycle hooks) | ✅ (delegate impl) | ✅ (tracking) |
| 3: Rendering | ✅ (rendering hooks) | ✅ (Metal backend) | ✅ (tracking) |
| 4: Shell | ✅ (shell hooks) | ✅ (command handling) | ✅ (tracking) |
| 5: I/O | ✅ (I/O hooks) | ✅ (security policy) | ✅ (tracking) |
| 6: Parport | ✅ (parport migration) | ❌ | ✅ (tracking) |
| 7: Input/Audio | ✅ (input hooks) | ✅ (input/MIDI impl) | ✅ (tracking) |
| 8: Testing | ✅ (fixes) | ✅ (fixes) | ✅ (tracking) |

---

## FIRST STEP: Verify Repository Setup

**⚠️ ALWAYS do this when starting a new Claude session:**

### Check if Repositories Are Cloned

```bash
ls -la /home/user/dosbox-staging-boxer/src/
```

You should see three directories:
- `dosbox-staging/`
- `boxer/`
- `dosbox-staging-legacy/`

### If Repositories Are Missing (BLOCKER-001)

If any directory is missing, clone them:

```bash
cd /home/user/dosbox-staging-boxer/src

# DOSBox Staging fork
git clone https://github.com/alecristia/dosbox-staging.git dosbox-staging
cd dosbox-staging
git checkout dosbox-boxer-upgrade-dosboxside
cd ..

# Boxer fork
git clone https://github.com/alecristia/Boxer.git boxer
cd boxer
git checkout boxer-dosbox-upgrade-boxerside
cd ..

# Legacy DOSBox (reference only)
git clone https://github.com/dosbox-staging/dosbox-staging.git dosbox-staging-legacy
cd dosbox-staging-legacy
git checkout main
cd ..
```

### Verify Branches

After cloning (or if already cloned), verify correct branches:

```bash
# Check DOSBox Staging branch
cd /home/user/dosbox-staging-boxer/src/dosbox-staging
git branch
# Should show: * dosbox-boxer-upgrade-dosboxside

# Check Boxer branch
cd /home/user/dosbox-staging-boxer/src/boxer
git branch
# Should show: * boxer-dosbox-upgrade-boxerside

# Check Legacy branch
cd /home/user/dosbox-staging-boxer/src/dosbox-staging-legacy
git branch
# Should show: * main
```

If repos exist but wrong branch, checkout the correct one.

**This corresponds to BLOCKER-001 in MASTER_ORCHESTRATOR.md.**

---

## Quick Reference

**Read orchestrator instructions:**
```
Read: /home/user/dosbox-staging-boxer/instructions/MASTER_ORCHESTRATOR.md
```

**Check current phase:**
```
Read: /home/user/dosbox-staging-boxer/PROGRESS.md
```

**View phase objectives:**
```
Read: /home/user/dosbox-staging-boxer/progress/phase-1/OBJECTIVES.md
```

**Create patch files:**
```
See: /home/user/dosbox-staging-boxer/instructions/PHASE_COMPLETION_TEMPLATE.md
```
