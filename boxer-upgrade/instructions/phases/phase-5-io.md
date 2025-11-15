# Phase 5: File I/O & Security - Agent Tasks

**Phase Duration**: Weeks 9-10
**Total Estimated Hours**: 36-56 hours
**Goal**: File access control and security enforced

**Prerequisites**: Phase 4 complete (shell integration working)

**🔒 SECURITY CRITICAL PHASE** - Careful attention to access control

---

## IMPORTANT: Repository Structure

**Root**: `/home/user/dosbox-staging-boxer/boxer-upgrade/`

**Two SEPARATE Repositories**:
1. DOSBox Staging (`src/dosbox-staging/`) - Branch: `dosbox-boxer-upgrade-dosboxside`
2. Boxer (`src/boxer/`) - Branch: `boxer-dosbox-upgrade-boxerside`

**Phase 5 modifies**: Both DOSBox Staging (file I/O hooks) and Boxer (security policy implementation)

---

## PHASE 5 OVERVIEW

By the end of Phase 5, you will have:
- DOS cannot write to protected areas
- .DS_Store and metadata files hidden from DOS
- Drive mount/unmount tracked
- File operations audited
- Boxer game bundles protected

---

## CRITICAL INTEGRATION POINTS

From integration-overview.md:
- **INT-039**: shouldMountPath
- **INT-040**: shouldShowFileWithName (MAJOR)
- **INT-041**: shouldAllowWriteAccessToPath (CRITICAL - security)
- **INT-042**: driveDidMount
- **INT-043**: driveDidUnmount
- **INT-044**: didCreateLocalFile
- **INT-045**: didRemoveLocalFile
- **INT-046-056**: File operation wrappers (many unused - consider removal)

---

## TASK 5-1: Security Model Design

### Context
- **Phase**: 5
- **Estimated Hours**: 6-8 hours
- **Criticality**: CRITICAL
- **Risk Level**: HIGH

### Objective
Design the security model for file access control - what can DOS write, what's protected.

### Prerequisites
- [ ] Phase 4 complete
- [ ] Understand Boxer's game bundle structure

### Input Documents
1. `src/boxer/Boxer/BXDrive.m`
   - Drive types (local, CD-ROM, floppy)
   - Path resolution

2. `src/boxer/Boxer/BXEmulator+BXDOSFileSystem.mm`
   - Current file system protection logic

3. `analysis/03-reintegration-analysis/unavoidable-modifications.md` lines 493-600

### Deliverables
1. **Design document**: `progress/phase-5/SECURITY_MODEL.md`
   - What paths are writable
   - What paths are read-only
   - What files are hidden
   - Edge cases and exceptions
   
2. **Documentation**: `progress/phase-5/tasks/TASK-5-1.md`

### Security Policies
1. **Game bundle contents**: Read-only (unless save game folder)
2. **macOS system directories**: Never writable
3. **User home**: Limited write access
4. **Temporary folders**: Writable
5. **Metadata files**: Hidden (.DS_Store, ._*, etc.)

### Success Criteria
- [ ] Complete security model documented
- [ ] All edge cases identified
- [ ] Boxer team has reviewed policy
- [ ] No security holes

---

## TASK 5-2: Write Access Control (INT-041)

### Context
- **Phase**: 5
- **Estimated Hours**: 10-14 hours
- **Criticality**: CRITICAL
- **Risk Level**: HIGH

### Objective
Implement shouldAllowWriteAccessToPath - the core security hook.

### Prerequisites
- [ ] TASK 5-1 complete (security model designed)

### Deliverables
1. **Modified**: `src/dosbox-staging/src/dos/drive_local.cpp`
   - Add BOXER_HOOK before write operations
   - Check permission before file create/modify/delete
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement shouldAllowWriteAccessToPath()
   - Apply security policy
   - Log access attempts
   
3. **Test**: Security test suite
   - Attempt write to protected path
   - Verify denial
   - Test all write operations
   
4. **Documentation**: `progress/phase-5/tasks/TASK-5-2.md`

### Implementation Pattern

```cpp
// In drive_local.cpp
bool localDrive::FileCreate(const char* name, ...) {
    #ifdef BOXER_INTEGRATED
    // Convert DOS path to host path
    char host_path[CROSS_LEN];
    GetHostPath(name, host_path);
    
    // Check with Boxer
    if (!BOXER_HOOK_BOOL(shouldAllowWriteAccessToPath, host_path)) {
        DOS_SetError(DOSERR_ACCESS_DENIED);
        return false;
    }
    #endif
    
    // Proceed with file creation
    // ...
}
```

### Security Testing

```bash
# Test cases
# 1. Attempt to write to /System - MUST FAIL
# 2. Attempt to write to game bundle - MUST FAIL (except saves)
# 3. Attempt to write to temp folder - MUST SUCCEED
# 4. Attempt to write to user-allowed folder - MUST SUCCEED
```

### Success Criteria
- [ ] Write attempts to protected paths blocked
- [ ] Write attempts to allowed paths succeed
- [ ] All write operations checked (create, modify, delete, rename)
- [ ] Access denials logged
- [ ] No bypasses possible

---

## TASK 5-3: File Visibility Filtering (INT-040)

### Context
- **Phase**: 5
- **Estimated Hours**: 8-10 hours
- **Criticality**: MAJOR
- **Risk Level**: MEDIUM

### Objective
Hide macOS metadata files from DOS directory listings.

### Prerequisites
- [ ] TASK 5-2 complete (write access works)

### Deliverables
1. **Modified**: `src/dosbox-staging/src/dos/drive_cache.cpp`
   - Add BOXER_HOOK when listing directory
   - Filter out files Boxer wants hidden
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement shouldShowFileWithName()
   - Hide .DS_Store, ._, Icon\r, etc.
   
3. **Test**: Directory listing test
   
4. **Documentation**: `progress/phase-5/tasks/TASK-5-3.md`

### Files to Hide
- `.DS_Store`
- `._*` (AppleDouble files)
- `.Spotlight-*`
- `.Trashes`
- `.fseventsd`
- `Icon\r`
- `.localized`

### Implementation Pattern

```cpp
// In drive_cache.cpp during directory enumeration
bool DOS_Drive_Cache::ReadDir(char* dirName, char* entry) {
    while (GetNextDirEntry(entry)) {
        #ifdef BOXER_INTEGRATED
        if (!BOXER_HOOK_BOOL(shouldShowFileWithName, entry)) {
            continue; // Skip this file
        }
        #endif
        return true;
    }
    return false;
}
```

### Success Criteria
- [ ] .DS_Store not visible in DIR listing
- [ ] Other metadata files hidden
- [ ] Regular files visible
- [ ] No pattern matching bugs

---

## TASK 5-4: Drive Mount/Unmount Callbacks

### Context
- **Phase**: 5
- **Estimated Hours**: 6-8 hours
- **Criticality**: MINOR
- **Risk Level**: LOW

### Objective
Notify Boxer when drives are mounted or unmounted.

### Prerequisites
- [ ] TASK 5-3 complete (file visibility works)

### Deliverables
1. **Modified**: DOSBox drive mounting code
   - Add BOXER_HOOK after mount succeeds
   - Add BOXER_HOOK after unmount
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement driveDidMount()
   - Implement driveDidUnmount()
   
3. **Modified**: Boxer UI code
   - Update drive list when mount/unmount occurs
   
4. **Documentation**: `progress/phase-5/tasks/TASK-5-4.md`

### Success Criteria
- [ ] Mount events trigger callback
- [ ] Unmount events trigger callback
- [ ] Drive letter and path passed correctly
- [ ] UI updates properly

---

## TASK 5-5: File Operation Notifications

### Context
- **Phase**: 5
- **Estimated Hours**: 4-6 hours
- **Criticality**: MINOR
- **Risk Level**: LOW

### Objective
Notify Boxer when files are created or deleted (for game library tracking).

### Prerequisites
- [ ] TASK 5-4 complete (mount/unmount works)

### Deliverables
1. **Modified**: `src/dosbox-staging/src/dos/drive_local.cpp`
   - Add BOXER_HOOK after file create
   - Add BOXER_HOOK after file delete
   
2. **Modified**: `src/boxer/Boxer/BoxerDelegate.mm`
   - Implement didCreateLocalFile()
   - Implement didRemoveLocalFile()
   
3. **Documentation**: `progress/phase-5/tasks/TASK-5-5.md`

### Use Cases
1. Game creates save file → Boxer notes it exists
2. User deletes file from DOS → Boxer updates game library
3. Game installs to disk → Boxer tracks installation

### Success Criteria
- [ ] File creation notifications received
- [ ] File deletion notifications received
- [ ] Path information correct
- [ ] No performance impact

---

## TASK 5-6: Remove Unused Wrappers

### Context
- **Phase**: 5
- **Estimated Hours**: 2-4 hours
- **Criticality**: LOW
- **Risk Level**: LOW

### Objective
Remove the 11 file operation wrappers (INT-046-056) that are declared but never used.

### Prerequisites
- [ ] TASK 5-5 complete (all used hooks working)
- [ ] Confirmed these are not needed

### Input Documents
1. `analysis/01-current-integration/integration-overview.md` lines 110-120
   - Many marked "Not directly used in boxer version"

### Deliverables
1. **Modified**: `src/dosbox-staging/include/boxer/boxer_hooks.h`
   - Remove unused function declarations
   - Document why removed
   
2. **Verification**: Confirm no calls exist
   
3. **Documentation**: `progress/phase-5/tasks/TASK-5-6.md`

### Functions to Remove (if truly unused)
- boxer_openLocalFile
- boxer_removeLocalFile
- boxer_moveLocalFile
- boxer_removeLocalDir
- boxer_getLocalPathStats
- boxer_localDirectoryExists
- boxer_localFileExists
- boxer_openLocalDirectory
- boxer_closeLocalDirectory
- boxer_getNextDirectoryEntry

### Success Criteria
- [ ] Verified functions are not called anywhere
- [ ] Removed from header
- [ ] No build errors
- [ ] Reduced integration surface area

---

## TASK 5-7: Security Audit

### Context
- **Phase**: 5
- **Estimated Hours**: 6-10 hours
- **Criticality**: CRITICAL
- **Risk Level**: LOW

### Objective
Audit all file operations for security holes.

### Prerequisites
- [ ] TASK 5-6 complete (cleanup done)

### Deliverables
1. **Audit report**: `progress/phase-5/SECURITY_AUDIT.md`
   - Every write path checked
   - Every file visibility check audited
   - Potential bypasses identified
   
2. **Test suite**: Comprehensive security tests
   - Attempt all known bypasses
   - Test symlink attacks
   - Test path traversal
   
3. **Documentation**: `progress/phase-5/tasks/TASK-5-7.md`

### Audit Checklist
- [ ] FileCreate checks permissions
- [ ] FileOpen (write mode) checks permissions
- [ ] FileDelete checks permissions
- [ ] DirectoryCreate checks permissions
- [ ] DirectoryRemove checks permissions
- [ ] FileRename checks permissions (both paths)
- [ ] No TOCTOU vulnerabilities
- [ ] Symlinks handled safely
- [ ] Path traversal blocked

### Success Criteria
- [ ] Complete audit documented
- [ ] No security holes found (or all fixed)
- [ ] Test suite passes
- [ ] Ready for production

---

## PHASE 5 COMPLETION CHECKLIST

### Access Control ✅
- [ ] Write access properly controlled
- [ ] Protected paths blocked
- [ ] Allowed paths work
- [ ] All operations checked

### File Visibility ✅
- [ ] Metadata files hidden
- [ ] Regular files visible
- [ ] No false positives/negatives

### Notifications ✅
- [ ] Mount/unmount tracked
- [ ] File create/delete tracked
- [ ] UI updates correctly

### Security ✅
- [ ] Security audit passed
- [ ] No bypasses found
- [ ] Path traversal blocked
- [ ] Symlinks safe

### Cleanup ✅
- [ ] Unused wrappers removed
- [ ] Code simplified
- [ ] Documentation updated

**When all boxes checked, Phase 5 is complete. Ready for Phase 6 (Parport).**

---

## ESTIMATED TIME BREAKDOWN

- TASK 5-1: Security Model Design - 6-8 hours
- TASK 5-2: Write Access Control - 10-14 hours
- TASK 5-3: File Visibility Filtering - 8-10 hours
- TASK 5-4: Drive Mount/Unmount - 6-8 hours
- TASK 5-5: File Operation Notifications - 4-6 hours
- TASK 5-6: Remove Unused Wrappers - 2-4 hours
- TASK 5-7: Security Audit - 6-10 hours

**Total**: 42-60 hours (~36-56 planned)

**Calendar time**: 1.5-2 weeks