# Phase Completion Template

Use this template at the end of EVERY phase to create patch files and instructions.

---

## End of Phase Checklist

When a phase is complete, the orchestrator must:

### 1. Identify Which Repositories Were Modified

Check which repositories have uncommitted or unpushed changes:

```bash
# Check DOSBox Staging
cd /home/user/dosbox-staging-boxer/boxer-upgrade/src/dosbox-staging
git status

# Check Boxer
cd /home/user/dosbox-staging-boxer/boxer-upgrade/src/boxer
git status
```

### 2. Create Patch Files (One Per Repository)

For EACH repository that was modified during the phase:

#### If DOSBox Staging was modified:

```bash
cd /home/user/dosbox-staging-boxer/boxer-upgrade/src/dosbox-staging

# Count unpushed commits
git log --oneline origin/dosbox-boxer-upgrade-dosboxside..HEAD

# Create patch file with all unpushed commits
git format-patch origin/dosbox-boxer-upgrade-dosboxside..HEAD --stdout > \
  /home/user/dosbox-staging-boxer/boxer-upgrade/phase-X-dosbox-changes.patch

# Verify patch was created
ls -lh /home/user/dosbox-staging-boxer/boxer-upgrade/phase-X-dosbox-changes.patch
```

#### If Boxer was modified:

```bash
cd /home/user/dosbox-staging-boxer/boxer-upgrade/src/boxer

# Count unpushed commits
git log --oneline origin/boxer-dosbox-upgrade-boxerside..HEAD

# Create patch file with all unpushed commits
git format-patch origin/boxer-dosbox-upgrade-boxerside..HEAD --stdout > \
  /home/user/dosbox-staging-boxer/boxer-upgrade/phase-X-boxer-changes.patch

# Verify patch was created
ls -lh /home/user/dosbox-staging-boxer/boxer-upgrade/phase-X-boxer-changes.patch
```

### 3. Commit Patch Files to Main Repository

```bash
cd /home/user/dosbox-staging-boxer

# Add the patch file(s)
git add boxer-upgrade/phase-X-*.patch

# Commit with descriptive message
git commit -m "Phase X: Add patch files for repository changes

DOSBox Staging patches: phase-X-dosbox-changes.patch (if created)
Boxer patches: phase-X-boxer-changes.patch (if created)

See PUSH-INSTRUCTIONS-PHASE-X.md for application instructions.
"
```

### 4. Create Push Instructions

Create `PUSH-INSTRUCTIONS-PHASE-X.md` with instructions for EACH repository modified:

**Template**:

```markdown
# Phase X: Push Instructions

## Repositories Modified

This phase modified the following repositories:

- [ ] DOSBox Staging (`eduo/dosbox-staging`, branch: `dosbox-boxer-upgrade-dosboxside`)
- [ ] Boxer (`eduo/Boxer`, branch: `boxer-dosbox-upgrade-boxerside`)

---

## DOSBox Staging Changes (if applicable)

**Patch File**: `phase-X-dosbox-changes.patch`
**Commits**: [list commit count and descriptions]

### Apply to DOSBox Staging:

1. Navigate to your local dosbox-staging repository:
   ```bash
   cd /path/to/your/dosbox-staging
   git checkout dosbox-boxer-upgrade-dosboxside
   ```

2. Download the patch:
   ```bash
   curl -O https://raw.githubusercontent.com/eduo/dosbox-staging-boxer/claude/phase-1-orchestrator-01H1B4jhqBRczSnhhSghWmN6/boxer-upgrade/phase-X-dosbox-changes.patch
   ```

3. Apply the patch:
   ```bash
   git am < phase-X-dosbox-changes.patch
   ```

4. Verify and push:
   ```bash
   git log --oneline -5
   git push origin dosbox-boxer-upgrade-dosboxside
   ```

---

## Boxer Changes (if applicable)

**Patch File**: `phase-X-boxer-changes.patch`
**Commits**: [list commit count and descriptions]

### Apply to Boxer:

1. Navigate to your local Boxer repository:
   ```bash
   cd /path/to/your/Boxer
   git checkout boxer-dosbox-upgrade-boxerside
   ```

2. Download the patch:
   ```bash
   curl -O https://raw.githubusercontent.com/eduo/dosbox-staging-boxer/claude/phase-1-orchestrator-01H1B4jhqBRczSnhhSghWmN6/boxer-upgrade/phase-X-boxer-changes.patch
   ```

3. Apply the patch:
   ```bash
   git am < phase-X-boxer-changes.patch
   ```

4. Verify and push:
   ```bash
   git log --oneline -5
   git push origin boxer-dosbox-upgrade-boxerside
   ```

---

## Files Modified Summary

### In DOSBox Staging:
- [list all files with line counts]

### In Boxer:
- [list all files with line counts]
```

### 5. Update PROGRESS.md

Add a note about the patch files:

```markdown
**Git Status**:
- ✅ Phase X work committed to main repository
- ⏳ DOSBox Staging changes: Apply `phase-X-dosbox-changes.patch` (see PUSH-INSTRUCTIONS-PHASE-X.md)
- ⏳ Boxer changes: Apply `phase-X-boxer-changes.patch` (see PUSH-INSTRUCTIONS-PHASE-X.md)
```

---

## Example: Phase 1 Pattern

Phase 1 only modified DOSBox Staging, so:
- ✅ Created `phase-1-dosbox-changes.patch`
- ✅ Created `PUSH-INSTRUCTIONS.md`
- ❌ No Boxer patch needed (no changes to Boxer)

Future phases may modify both repositories, requiring TWO patch files.

---

## Quick Reference

**Always check both repositories** at end of phase:
```bash
cd /home/user/dosbox-staging-boxer/boxer-upgrade

# DOSBox Staging status
cd src/dosbox-staging && git status && cd ../..

# Boxer status
cd src/boxer && git status && cd ../..
```

**Create patches only for repositories with unpushed commits.**

**File naming convention**:
- `phase-X-dosbox-changes.patch` - DOSBox Staging changes
- `phase-X-boxer-changes.patch` - Boxer changes
- `PUSH-INSTRUCTIONS-PHASE-X.md` - Instructions for phase X

---

## Why This Process?

Claude Code Web's git proxy can only push to the main `dosbox-staging-boxer` repository. The `src/dosbox-staging/` and `src/boxer/` repositories are separate git repos that require manual push from the user's local machine with proper GitHub credentials.

Patch files allow the user to apply all commits from the web session to their local repositories and push them to GitHub.
