# REPOSITORY STRUCTURE - CRITICAL INFORMATION

**Add this section at the beginning of EVERY phase template**

---

## IMPORTANT: Repository Structure and Paths

### Working Directory Root
**All paths in this document are relative to**: `/home/user/dosbox-staging-boxer/boxer-upgrade/`

When you see paths like:
- `src/dosbox-staging/CMakeLists.txt`
- `src/boxer/Boxer/BXEmulator.mm`
- `analysis/01-current-integration/...`

The **FULL absolute paths** are:
- `/home/user/dosbox-staging-boxer/boxer-upgrade/src/dosbox-staging/CMakeLists.txt`
- `/home/user/dosbox-staging-boxer/boxer-upgrade/src/boxer/Boxer/BXEmulator.mm`
- `/home/user/dosbox-staging-boxer/boxer-upgrade/analysis/01-current-integration/...`

### Two SEPARATE Git Repositories

**CRITICAL**: `src/dosbox-staging/` and `src/boxer/` are **SEPARATE git repositories** with their own branches and remotes:

#### 1. DOSBox Staging Repository
- **Location**: `src/dosbox-staging/`
- **Full Path**: `/home/user/dosbox-staging-boxer/boxer-upgrade/src/dosbox-staging/`
- **Remote**: `https://github.com/eduo/dosbox-staging.git`
- **Branch**: `dosbox-boxer-upgrade-dosboxside`
- **Purpose**: All DOSBox source code modifications

#### 2. Boxer Repository
- **Location**: `src/boxer/`
- **Full Path**: `/home/user/dosbox-staging-boxer/boxer-upgrade/src/boxer/`
- **Remote**: `https://github.com/eduo/Boxer.git`
- **Branch**: `boxer-dosbox-upgrade-boxerside`
- **Purpose**: All Boxer source code modifications

### When Modifying Files

**ALWAYS be aware which repository you're modifying:**

**Modifying DOSBox files** (CMakeLists.txt, dosbox.cpp, headers, etc.):
```bash
cd /home/user/dosbox-staging-boxer/boxer-upgrade/src/dosbox-staging
# Make changes
git add [files]
git commit -m "message"
# Note: Cannot push directly from web environment - will need patch file
```

**Modifying Boxer files** (BXEmulator.mm, etc.):
```bash
cd /home/user/dosbox-staging-boxer/boxer-upgrade/src/boxer
# Make changes
git add [files]
git commit -m "message"
# Note: Cannot push directly from web environment - will need patch file
```

**Modifying project structure** (progress/, validation/, etc.):
```bash
cd /home/user/dosbox-staging-boxer
# Make changes to boxer-upgrade/...
git add boxer-upgrade/[files]
git commit -m "message"
git push origin [branch]
```

### End of Phase: Create Patch Files

At the end of EVERY phase, check BOTH repositories for unpushed changes and create patch files:

```bash
# Check DOSBox Staging
cd /home/user/dosbox-staging-boxer/boxer-upgrade/src/dosbox-staging
git log --oneline origin/dosbox-boxer-upgrade-dosboxside..HEAD
# If commits exist, create patch

# Check Boxer
cd /home/user/dosbox-staging-boxer/boxer-upgrade/src/boxer
git log --oneline origin/boxer-dosbox-upgrade-boxerside..HEAD
# If commits exist, create patch
```

See `instructions/PHASE_COMPLETION_TEMPLATE.md` for detailed patch creation instructions.

---
