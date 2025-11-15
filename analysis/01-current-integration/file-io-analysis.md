# File I/O & Drive Management Analysis

**Agent**: Agent 1B.4 - File I/O & Drive Management
**Created**: 2025-11-14T14:30:00Z
**Status**: Completed
**Dependencies**: Agent 1A

## Summary

The File I/O integration points represent **CRITICAL** functionality for Boxer's security model and file access control. The target DOSBox Staging has undergone substantial architectural modernization:

- **Native file handle abstraction**: Uses `NativeFileHandle` instead of raw `FILE*` pointers
- **Read-only drive support**: Built-in `readonly` flag at the drive level (new architecture)
- **Attribute system modernization**: Replaced `Bit16u` with `FatAttributeFlags` struct
- **Modern C++ patterns**: `std::unique_ptr`, `std::weak_ptr`, `std::filesystem::path`
- **No Boxer callbacks**: All 18 integration points are absent in target (as expected)

**Migration Effort**: 40-60 hours (HIGH complexity due to architectural changes)
**Critical Risk**: INT-041 (write access control) requires careful re-implementation

---

## Legacy File I/O Architecture

### localDrive Class Structure

**Location**: `/home/user/dosbox-staging-boxer/src/dos/drive_local.cpp`
**Header**: `/home/user/dosbox-staging-boxer/include/drives.h`

```cpp
class localDrive : public DOS_Drive {
    char basedir[CROSS_LEN];
    std::unordered_set<std::string> write_protected_files;

    // Key Methods:
    bool FileCreate(DOS_File **file, char *name, Bit16u attributes);
    bool FileOpen(DOS_File **file, char *name, Bit32u flags);
    bool FileUnlink(char *name);
    bool MakeDir(char *dir);
};

class localFile : public DOS_File {
    FILE *fhandle;
    char basedir[CROSS_LEN];
    bool read_only_medium;
};
```

### Key Characteristics:
- Uses raw `FILE*` pointers (`fhandle`)
- Raw C-style `char[]` arrays
- Direct `fopen`, `fread`, `fwrite`, `fclose` calls
- Manual memory management for `DOS_File*` (raw pointers)
- Attribute system uses `Bit16u` bitmasks

---

## Target File I/O Architecture

### Modernized localDrive Class

**Location**: `/home/user/dosbox-staging/src/dos/drive_local.cpp`
**Header**: `/home/user/dosbox-staging/src/dos/drives.h`

```cpp
// Requires std::enable_shared_from_this - must be constructed with shared_ptr
class localDrive : public DOS_Drive,
                   public std::enable_shared_from_this<localDrive> {
public:
    localDrive(const char* startdir, uint16_t _bytes_sector,
               uint8_t _sectors_cluster, uint16_t _total_clusters,
               uint16_t _free_clusters, uint8_t _mediaid,
               bool _readonly,  // NEW: Built-in read-only support
               bool _always_open_ro_files = false);

    // Returns unique_ptr instead of raw pointer
    std::unique_ptr<DOS_File> FileCreate(const char* name,
                                         FatAttributeFlags attributes);
    std::unique_ptr<DOS_File> FileOpen(const char* name, uint8_t flags);

    bool IsReadOnly() const override { return readonly; }

    std::string MapDosToHostFilename(const char* const dos_name);

    // NEW: Timestamp caching for DOS time semantics
    std::unordered_map<std::string, DosDateTime> timestamp_cache = {};

private:
    const bool readonly;  // NEW: Const member
    const bool always_open_ro_files;
    std::unordered_set<std::string> write_protected_files;
};

class localFile : public DOS_File {
    NativeFileHandle file_handle;  // NEW: Abstraction instead of FILE*
    const std_fs::path path;        // NEW: std::filesystem::path
    const std::weak_ptr<localDrive> local_drive;  // NEW: Weak reference
    const bool read_only_medium;
    bool set_archive_on_close = false;
};
```

### Major Architectural Changes:

1. **Native File Handle Abstraction**
   - `NativeFileHandle` replaces `FILE*`
   - Platform-agnostic API: `open_native_file()`, `read_native_file()`, `write_native_file()`
   - Defined in: `/home/user/dosbox-staging/include/misc/cross.h`

2. **Smart Pointers Throughout**
   - `std::unique_ptr<DOS_File>` return types (no manual delete)
   - `std::weak_ptr<localDrive>` for parent references (prevents cycles)
   - `std::enable_shared_from_this` for safe weak references

3. **Modern Path Handling**
   - `std::filesystem::path` (C++17) instead of `char[]`
   - `MapDosToHostFilename()` returns `std::string`

4. **Built-in Read-Only Support**
   - `readonly` flag at drive construction
   - `IsReadOnly()` virtual method
   - `assert(!IsReadOnly())` guards on write operations

5. **Attribute System Modernization**
   - `FatAttributeFlags` type-safe struct replaces `Bit16u`
   - Member access: `attr.read_only`, `attr.hidden`, `attr.system`, etc.

---

## Integration Point Analysis

### CRITICAL: INT-041: boxer_shouldAllowWriteAccessToPath

**SECURITY CRITICAL** - Controls write permission to host filesystem

**Legacy Implementation**:
- **Location**: `drive_local.cpp:60, 145, 265, 450`
- **Purpose**: Allow Boxer to selectively deny write access to files/directories
- **Signature**: `bool boxer_shouldAllowWriteAccessToPath(const char *filePath, DOS_Drive *dosboxDrive)`

**Call Sites** (4 locations):

1. **Line 60 - FileCreate()**:
   ```cpp
   if (!boxer_shouldAllowWriteAccessToPath((const char *)newname, this)) {
       DOS_SetError(DOSERR_ACCESS_DENIED);
       return false;
   }
   ```

2. **Line 145 - FileOpen() with write access**:
   ```cpp
   if (!strcmp(type, "rb+")) {  // Write mode
       if (!boxer_shouldAllowWriteAccessToPath((const char *)newname, this)) {
           // Try to downgrade to read-only or deny
           if ((flags&3)==OPEN_READWRITE) {
               flags &= ~OPEN_READWRITE;
           } else {
               DOS_SetError(DOSERR_ACCESS_DENIED);
               return false;
           }
       }
   }
   ```

3. **Line 265 - FileUnlink()**:
   ```cpp
   if (!boxer_shouldAllowWriteAccessToPath((const char *)fullname, this)) {
       DOS_SetError(DOSERR_ACCESS_DENIED);
       return false;
   }
   ```

4. **Line 450 - MakeDir()**:
   ```cpp
   if (!boxer_shouldAllowWriteAccessToPath(dirCache.GetExpandName(newdir), this)) {
       DOS_SetError(DOSERR_ACCESS_DENIED);
       return false;
   }
   ```

**Target Equivalent**:
- **Status**: NONE - Target has no equivalent callback mechanism
- **Built-in Alternative**: Drive-level `readonly` flag
- **Coverage**: Drive-level only, cannot selectively protect individual files

**Migration Strategy**:
1. **Option A - Hook Injection Points** (Recommended):
   - Inject checks at same 4 locations in target code
   - Use Boxer's security policy to determine access
   - Similar pattern to legacy implementation

2. **Option B - Subclass Override**:
   - Create `BoxerLocalDrive : public localDrive`
   - Override: `FileCreate()`, `FileOpen()`, `FileUnlink()`, `MakeDir()`
   - Less invasive to target codebase

3. **Option C - Virtual Method Addition**:
   - Add virtual `bool ShouldAllowWriteAccess(const char* path)` to `DOS_Drive`
   - Default returns `!IsReadOnly()`
   - Override in Boxer's drive class

**Recommended**: Option A (direct injection) for maximum compatibility with target's architecture.

---

### Drive Management Callbacks

#### INT-039: boxer_shouldMountPath

**Legacy Implementation**:
- **Location**: NOT FOUND in dos_programs.cpp (may be in different file)
- **Purpose**: Verify that DOSBox is allowed to mount specified folder
- **Signature**: `bool boxer_shouldMountPath(const char *filePath)`

**Search Results**: No calls found in `/home/user/dosbox-staging-boxer/src/dos/`

**Status**: **LIKELY UNUSED** - Declared in BXCoalface.h but not called from legacy code

**Target Equivalent**: NONE

**Migration**: LOW PRIORITY - May not need migration if unused

---

#### INT-042: boxer_driveDidMount

**Legacy Implementation**:
- **Location**: `program_mount.cpp:387`
- **Purpose**: Notify Boxer of drive mount events
- **Signature**: `void boxer_driveDidMount(Bit8u driveIndex)`
- **Call Site**:
  ```cpp
  // After successful mount
  boxer_driveDidMount(drive-'A');  // Line 387
  ```

**Target Equivalent**: NONE

**Target Location**: `/home/user/dosbox-staging/src/dos/programs/mount.cpp`

**Migration Strategy**:
- Inject call after drive mounting in target's mount command
- Search for: `Drives[drive_index(drive)] = newdrive;`
- Add callback immediately after assignment

**Complexity**: LOW

---

#### INT-043: boxer_driveDidUnmount

**Legacy Implementation**:
- **Location**: `program_mount_common.cpp:66`
- **Purpose**: Notify Boxer of drive unmount events
- **Signature**: `void boxer_driveDidUnmount(Bit8u driveIndex)`
- **Call Site**:
  ```cpp
  // After successful unmount
  boxer_driveDidUnmount(i_drive);  // Line 66
  ```

**Target Equivalent**: NONE

**Target Location**: `/home/user/dosbox-staging/src/dos/programs/mount_common.cpp`

**Migration Strategy**:
- Inject call in unmount helper function
- Similar to legacy location

**Complexity**: LOW

---

#### INT-040: boxer_shouldShowFileWithName

**Legacy Implementation**:
- **Location**: `drive_cache.cpp:804`
- **Purpose**: Allow Boxer to hide OS X metadata files (.DS_Store, ._* files)
- **Signature**: `bool boxer_shouldShowFileWithName(const char *name)`
- **Call Site**:
  ```cpp
  void DOS_Drive_Cache::CreateEntry(CFileInfo* dir, const char* name, bool is_directory) {
      if (!boxer_shouldShowFileWithName(name)) return;  // Line 804
      // ... proceed with creating entry
  }
  ```

**Target Equivalent**: NONE

**Target Location**: `/home/user/dosbox-staging/src/dos/drive_cache.cpp` (file exists)

**Target Alternative**: `is_hidden_by_host()` function (line 353 in drive_local.cpp)
- Already filters host-hidden files during `FindNext()`
- May provide partial coverage

**Migration Strategy**:
1. Find equivalent `CreateEntry` or file enumeration in target
2. Inject Boxer's file filtering logic
3. May need to extend beyond just drive_cache if architecture changed

**Complexity**: MEDIUM (need to verify target's caching architecture)

---

### File Operation Callbacks

#### INT-044: boxer_didCreateLocalFile

**Legacy Implementation**:
- **Location**: `drive_local.cpp:86`
- **Purpose**: Notify Boxer when DOSBox creates a local file
- **Signature**: `void boxer_didCreateLocalFile(const char *path, DOS_Drive *dosboxDrive)`
- **Call Site**:
  ```cpp
  // After successful file creation
  boxer_didCreateLocalFile(temp_name, this);  // Line 86
  ```

**Target Equivalent**: NONE

**Migration Strategy**:
- Inject at end of `localDrive::FileCreate()` in target
- Call after `dirCache.AddEntry()`

**Complexity**: LOW

---

#### INT-045: boxer_didRemoveLocalFile

**Legacy Implementation**:
- **Location**: `drive_local.cpp:276, 297`
- **Purpose**: Notify Boxer when DOSBox removes a local file
- **Signature**: `void boxer_didRemoveLocalFile(const char *path, DOS_Drive *dosboxDrive)`
- **Call Sites**:
  ```cpp
  // After successful file removal
  boxer_didRemoveLocalFile(fullname, this);  // Line 276, 297
  ```

**Target Equivalent**: NONE

**Migration Strategy**:
- Inject in `localDrive::FileUnlink()` in target
- Call after `dirCache.DeleteEntry()`

**Complexity**: LOW

---

### File Operation Wrappers (11 Functions)

**IMPORTANT FINDING**: All 11 wrapper functions are **DECLARED** in BXCoalface.h and **IMPLEMENTED** in BXCoalface.mm, but **NOT CALLED** from legacy DOSBox code.

| ID | Function | Declared | Implemented | Called from DOSBox |
|----|----------|----------|-------------|-------------------|
| INT-046 | boxer_openLocalFile | YES | YES | **NO** |
| INT-047 | boxer_removeLocalFile | YES | YES | **NO** |
| INT-048 | boxer_moveLocalFile | YES | YES | **NO** |
| INT-049 | boxer_createLocalDir | YES | YES | **YES** (line 460) |
| INT-050 | boxer_removeLocalDir | YES | YES | **NO** |
| INT-051 | boxer_getLocalPathStats | YES | YES | **NO** |
| INT-052 | boxer_localDirectoryExists | YES | YES | **NO** |
| INT-053 | boxer_localFileExists | YES | YES | **NO** |
| INT-054 | boxer_openLocalDirectory | YES | YES | **NO** |
| INT-055 | boxer_closeLocalDirectory | YES | YES | **NO** |
| INT-056 | boxer_getNextDirectoryEntry | YES | YES | **NO** |

#### Actually Used Wrapper

**INT-049: boxer_createLocalDir**

**Legacy Implementation**:
- **Location**: `drive_local.cpp:460`
- **Purpose**: Create directory with correct permissions via Boxer
- **Signature**: `bool boxer_createLocalDir(const char *path, DOS_Drive *drive)`
- **Call Site**:
  ```cpp
  bool created = boxer_createLocalDir(dirCache.GetExpandName(newdir), this);
  if (created) dirCache.CacheOut(newdir,true);
  return created;
  ```

**Target Equivalent**: `local_drive_create_dir()` helper function

**Migration Strategy**:
- Target already has proper abstraction: `local_drive_create_dir()`
- Boxer can wrap or hook this function
- Check if target's implementation sets correct permissions

**Complexity**: LOW

---

#### Unused Wrappers (10 Functions)

**Analysis**: These functions are implemented in Boxer but never called from DOSBox:
- **boxer_openLocalFile**: DOSBox uses `fopen_wrap()` directly
- **boxer_removeLocalFile**: DOSBox uses `remove()` directly
- **boxer_moveLocalFile**: DOSBox uses `rename()` directly
- **boxer_removeLocalDir**: DOSBox uses `rmdir()` directly
- **boxer_getLocalPathStats**: DOSBox uses `stat()` directly
- **boxer_localDirectoryExists**: DOSBox uses `path_exists()` directly
- **boxer_localFileExists**: DOSBox uses `stat()` directly
- **boxer_openLocalDirectory**: DOSBox uses `open_directory()` directly
- **boxer_closeLocalDirectory**: DOSBox uses `close_directory()` directly
- **boxer_getNextDirectoryEntry**: DOSBox uses `read_directory_*()` directly

**Recommendation**: **DO NOT MIGRATE** these wrappers unless Boxer has external code calling them.

---

## Target Architecture Deep Dive

### NativeFileHandle System

**Location**: `include/misc/cross.h` (assumed)

The target uses an abstraction layer for file I/O:

```cpp
// Type alias (platform-specific)
#ifdef _WIN32
using NativeFileHandle = HANDLE;
constexpr NativeFileHandle InvalidNativeFileHandle = INVALID_HANDLE_VALUE;
#else
using NativeFileHandle = int;  // File descriptor on Unix
constexpr NativeFileHandle InvalidNativeFileHandle = -1;
#endif

// API Functions
NativeFileHandle open_native_file(const std::string& path, bool write_access);
NativeFileHandle create_native_file(const std::string& path, FatAttributeFlags attributes);
bool delete_native_file(const std::string& path);

struct NativeReadResult {
    size_t num_bytes;
    bool error;
};
NativeReadResult read_native_file(NativeFileHandle handle, uint8_t* data, uint16_t size);

struct NativeWriteResult {
    size_t num_bytes;
    bool error;
};
NativeWriteResult write_native_file(NativeFileHandle handle, uint8_t* data, uint16_t size);

// Seeking
enum class NativeSeek { Set, Current, End };
constexpr int64_t NativeSeekFailed = -1;
int64_t seek_native_file(NativeFileHandle handle, int64_t offset, NativeSeek whence);
int64_t get_native_file_position(NativeFileHandle handle);

void close_native_file(NativeFileHandle handle);
bool truncate_native_file(NativeFileHandle handle);

// Timestamps
DosDateTime get_dos_file_time(NativeFileHandle handle);
void set_dos_file_time(NativeFileHandle handle, uint16_t date, uint16_t time);
```

**Key Benefits**:
- Platform abstraction (Windows HANDLE vs Unix FD)
- Consistent error handling
- DOS timestamp management
- Type safety

---

### FatAttributeFlags System

**Location**: `include/dos/dos.h` (assumed)

```cpp
struct FatAttributeFlags {
    uint8_t _data = 0;

    // Bit accessors
    bool read_only : 1;
    bool hidden : 1;
    bool system : 1;
    bool volume : 1;
    bool directory : 1;
    bool archive : 1;

    // Constructors
    FatAttributeFlags() = default;
    explicit FatAttributeFlags(uint8_t raw) : _data(raw) {}

    // Comparisons
    bool operator==(FatAttributeFlags other) const { return _data == other._data; }
    bool operator==(uint8_t raw) const { return _data == raw; }
};
```

**Migration Impact**:
- Replace all `Bit16u attr` with `FatAttributeFlags attr`
- Change bit manipulation: `attr & DOS_ATTR_READONLY` → `attr.read_only`
- Update function signatures

---

### Timestamp Caching Architecture

**New Feature in Target**: `localDrive::timestamp_cache`

```cpp
std::unordered_map<std::string, DosDateTime> timestamp_cache = {};
```

**Purpose**:
- DOS uses internal BIOS time, not host time
- Cache locks in timestamp when file first opened
- Multiple handles to same file see consistent time
- Flushed on file close

**Impact on Integration**:
- Callbacks receive paths that may be in cache
- `boxer_didCreateLocalFile()` should be aware of cache
- Cache cleared on file deletion

---

## Summary Table

| ID | Name | Used? | Legacy Loc | Target Status | Complexity |
|----|------|-------|------------|---------------|------------|
| **Drive Management** |
| INT-039 | shouldMountPath | NO | Not found | Missing | LOW (if needed) |
| INT-040 | shouldShowFileWithName | YES | cache:804 | Missing | MEDIUM |
| INT-042 | driveDidMount | YES | mount:387 | Missing | LOW |
| INT-043 | driveDidUnmount | YES | common:66 | Missing | LOW |
| **File Access Control (CRITICAL)** |
| INT-041 | shouldAllowWriteAccessToPath | **YES** | local:60,145,265,450 | **Missing** | **HIGH** |
| **File Operation Callbacks** |
| INT-044 | didCreateLocalFile | YES | local:86 | Missing | LOW |
| INT-045 | didRemoveLocalFile | YES | local:276,297 | Missing | LOW |
| **File Operation Wrappers** |
| INT-046 | openLocalFile | NO | None | Missing | N/A |
| INT-047 | removeLocalFile | NO | None | Missing | N/A |
| INT-048 | moveLocalFile | NO | None | Missing | N/A |
| INT-049 | createLocalDir | YES | local:460 | Alternative exists | LOW |
| INT-050 | removeLocalDir | NO | None | Missing | N/A |
| INT-051 | getLocalPathStats | NO | None | Missing | N/A |
| INT-052 | localDirectoryExists | NO | None | Missing | N/A |
| INT-053 | localFileExists | NO | None | Missing | N/A |
| INT-054 | openLocalDirectory | NO | None | Missing | N/A |
| INT-055 | closeLocalDirectory | NO | None | Missing | N/A |
| INT-056 | getNextDirectoryEntry | NO | None | Missing | N/A |

---

## Critical Findings

### Security Controls

**CRITICAL RISK**: INT-041 (shouldAllowWriteAccessToPath) is the **primary security mechanism** for Boxer's file access control. Without this:
- DOS programs could write to any file in mounted directories
- No selective file/directory protection
- Cannot protect game installations from save game corruption
- Cannot protect system folders

**Mitigation Required**: This integration point is **NON-NEGOTIABLE** and must be re-implemented.

---

### Drive Management

**STATUS**: Straightforward migration
- Mount/unmount notifications (INT-042, INT-043) are simple callbacks
- Injection points easy to identify in target
- No architectural changes needed

---

### File Operation Callbacks

**STATUS**: Low complexity
- Create/remove notifications (INT-044, INT-045) are observational only
- No impact on file operations themselves
- Target has equivalent injection points

---

## Migration Complexity

### Essential Integrations (Must Migrate)

| Integration | Effort | Priority | Risk |
|-------------|--------|----------|------|
| INT-041: shouldAllowWriteAccessToPath | 16-24h | **CRITICAL** | HIGH - Security essential |
| INT-040: shouldShowFileWithName | 8-12h | HIGH | MEDIUM - UX impact |
| INT-042: driveDidMount | 2-4h | MEDIUM | LOW - Simple callback |
| INT-043: driveDidUnmount | 2-4h | MEDIUM | LOW - Simple callback |
| INT-044: didCreateLocalFile | 2-4h | LOW | LOW - Notification only |
| INT-045: didRemoveLocalFile | 2-4h | LOW | LOW - Notification only |
| INT-049: createLocalDir | 4-6h | LOW | LOW - Alternative exists |

**Total Essential Effort**: 36-56 hours

---

### Optional Integrations (Can Skip)

| Integration | Status | Decision |
|-------------|--------|----------|
| INT-039: shouldMountPath | Not called in legacy | **SKIP** unless needed |
| INT-046-048, INT-050-056 | Not called in legacy | **SKIP** - Remove from BXCoalface.h |

**Recommendation**: Remove unused wrapper declarations from BXCoalface.h to reduce maintenance burden.

---

## Risks

### CRITICAL Risks

1. **INT-041 Migration Failure**:
   - **Risk**: If write access control cannot be implemented, major security vulnerability
   - **Impact**: DOS programs can modify/delete any files in mounted directories
   - **Mitigation**: Extensive testing of all 4 injection points
   - **Fallback**: Use drive-level readonly flag (reduced granularity)

2. **API Signature Changes**:
   - **Risk**: `DOS_Drive*` vs `std::shared_ptr<DOS_Drive>` incompatibility
   - **Impact**: Callback signatures may need adjustment
   - **Mitigation**: Create adapter layer if needed

---

### HIGH Risks

1. **NativeFileHandle Abstraction**:
   - **Risk**: Boxer code may expect `FILE*` pointers
   - **Impact**: Cannot use `ftell`, `fseek`, `fread` directly
   - **Mitigation**: Update Boxer to use target's API or add conversion layer

2. **FatAttributeFlags Migration**:
   - **Risk**: All attribute-related code needs updating
   - **Impact**: 100+ locations across Boxer codebase
   - **Mitigation**: Compile-time errors will catch most issues

---

### MEDIUM Risks

1. **Timestamp Cache Interaction**:
   - **Risk**: Boxer's file monitoring may see stale timestamps
   - **Impact**: File modification detection could be inaccurate
   - **Mitigation**: Understand cache invalidation rules

2. **Read-Only Architecture**:
   - **Risk**: Target's built-in readonly flag may conflict with selective protection
   - **Impact**: INT-041 implementation may need coordination with IsReadOnly()
   - **Mitigation**: Careful design of permission hierarchy

---

## Recommendations

### 1. Prioritize INT-041 (Critical Security)

**Action Plan**:
1. Study target's `FileCreate`, `FileOpen`, `FileUnlink`, `MakeDir` implementations
2. Design injection mechanism (direct injection vs. subclassing)
3. Implement write access callback
4. Extensive testing with:
   - Read-only files
   - Protected directories
   - Various DOS programs (installers, games with save files)

**Success Criteria**: DOS programs correctly denied access to protected paths

---

### 2. Clean Up Unused Wrappers

**Action Plan**:
1. Remove unused wrapper declarations from BXCoalface.h:
   - `boxer_openLocalFile` through `boxer_getNextDirectoryEntry`
   - Keep: `boxer_createLocalDir` (actually used)
2. Remove implementations from BXCoalface.mm
3. Document reason for removal in commit message

**Benefits**: Reduced maintenance burden, clearer API surface

---

### 3. Adapt to Modern C++ Patterns

**Action Plan**:
1. Update Boxer's DOSBox integration layer to handle:
   - `std::unique_ptr<DOS_File>` instead of `DOS_File**`
   - `std::shared_ptr<DOS_Drive>` for drive references
   - `std::string` instead of `char*` for paths
2. Create adapter functions if needed:
   ```cpp
   // Adapter for callbacks
   bool boxer_shouldAllowWriteAccessToPath_adapted(
       const std::string& path,
       std::shared_ptr<localDrive> drive)
   {
       // Convert to C types for existing Boxer code
       return boxer_shouldAllowWriteAccessToPath(
           path.c_str(),
           drive.get());
   }
   ```

---

### 4. Investigate File Hiding Mechanism

**Action Plan**:
1. Compare target's `is_hidden_by_host()` with legacy's INT-040
2. Determine if target already hides macOS metadata files
3. If not, implement INT-040 equivalent in target's file enumeration

**Testing**: Verify `.DS_Store` and `._*` files are hidden from DOS programs

---

### 5. Coordinate with Drive-Level Readonly

**Action Plan**:
1. Understand interaction between:
   - `localDrive::IsReadOnly()` (drive-level)
   - `boxer_shouldAllowWriteAccessToPath()` (file-level)
2. Design permission hierarchy:
   - Drive readonly → denies all writes
   - File-level protection → selective denial on writable drives
3. Document expected behavior

---

## Blockers/Open Questions

### Questions for Boxer Team

1. **Wrapper Function Usage**:
   - Are INT-046 through INT-056 (except INT-049) used by any Boxer code outside DOSBox?
   - Can they be safely removed, or are they part of a public API?

2. **Write Protection Policy**:
   - What criteria does Boxer use for `shouldAllowWriteAccessToPath`?
   - Is it based on file paths, file types, or runtime state?
   - Can the logic be shared as a document for testing?

3. **Mount Validation**:
   - Is INT-039 (`shouldMountPath`) actually needed?
   - If so, where should it be called from (not found in legacy)?

### Technical Unknowns

1. **NativeFileHandle Implementation**:
   - Need to verify exact API signatures in target codebase
   - Confirm error handling patterns
   - Check if `FILE*` conversion is ever needed

2. **FatAttributeFlags Structure**:
   - Need to verify actual implementation (assumed structure)
   - Check for helper methods or utilities
   - Confirm bit positions match DOS standard

3. **Drive Cache Architecture**:
   - Has target's drive_cache.cpp changed significantly?
   - Is `CreateEntry` still the right injection point for INT-040?
   - What is caching strategy in target?

---

## Next Steps

### Immediate Actions

1. **Agent 1B.5 (Integration): Read this document** to understand File I/O findings
2. **Agent 1C (Code Mapping)**: Map these integration points to actual target code locations
3. **Agent 1D (Gap Analysis)**: Assess if target's modern architecture enables or hinders these integrations

### Follow-Up Tasks

1. Create detailed implementation plan for INT-041 (critical security)
2. Verify NativeFileHandle API by reading target's cross.h
3. Verify FatAttributeFlags implementation by reading target's dos.h
4. Create test plan for write protection validation
5. Document Boxer's file protection policy for reference

---

## Conclusion

File I/O integration is **FEASIBLE** but requires significant effort due to architectural modernization. The critical security integration (INT-041) has clear injection points and can be preserved. The majority of wrapper functions are unused legacy code and can be removed.

**Key Takeaway**: Target's modern C++ patterns are **well-designed** and will improve code quality, but require careful adaptation of Boxer's callback mechanism.

**Confidence Level**: HIGH for essential integrations, MEDIUM for overall effort estimate.
