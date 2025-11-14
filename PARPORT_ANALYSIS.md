# Parallel Port Implementation Analysis

**Document Version:** 1.0
**Date:** 2025-11-14
**Purpose:** Detailed analysis of parallel port implementation and migration options

## Executive Summary

The parallel port subsystem in DOSBox Staging has been **completely removed** in version 0.83.0. The old implementation (version 0.78.0) included a full printer emulation system, of which Boxer used only a small portion - the `printer_redir` module that redirected all parallel port I/O to Boxer's own virtual printer implementation.

**Key Decision:** Whether to port forward the parport code or drop printer support entirely.

---

## Current Implementation (0.78.0)

### Directory Structure

```
src/hardware/parport/
├── parport.cpp/h               (1,823 lines) - Core parallel port infrastructure
├── printer.cpp/h               (2,081 lines) - Full ESC/P printer emulation
├── printer_redir.cpp/h         (118 lines)   - Boxer-specific redirection **KEY FILE**
├── filelpt.cpp/h               (200 lines)   - File output implementation
├── directlpt_win32.cpp/h       (400 lines)   - Windows hardware access
├── directlpt_linux.cpp/h       (300 lines)   - Linux hardware access
├── printer_charmaps.cpp/h      (1,500 lines) - Character set tables
└── printer_if.h                (50 lines)    - Interface definitions
```

**Total Code:** ~6,500 lines

### Class Hierarchy

```
CParallel (abstract base class)
    ├── CPrinter         - Full Epson ESC/P emulation
    ├── CPrinterRedir    - Boxer-specific redirection ⭐
    ├── CFileLpt         - Output to file
    └── CDirectLPT       - Direct hardware access
```

### Boxer's Usage

Boxer creates a `CPrinterRedir` instance that acts as a thin shim, forwarding all I/O operations to Boxer's Objective-C printer implementation via the `boxer_PRINTER_*` callback functions.

**Boxer ONLY uses:**
- `parport.cpp/h` - Core infrastructure (needed)
- `printer_redir.cpp/h` - Boxer integration (needed)
- `printer_if.h` - Interface definition (needed)

**Boxer does NOT use:**
- `printer.cpp/h` - Full ESC/P emulation
- `filelpt.cpp/h` - File output
- `directlpt_*.cpp/h` - Hardware access
- `printer_charmaps.cpp/h` - Character sets

---

## Boxer Integration Point

### printer_redir.cpp - Complete Source

```cpp
/*
 *  Copyright (C) 2002-2021  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "parport.h"

//--Added 2011-03-22 by Alun Bestor
#import "BXCoalface.h"
//--End of modifications

CPrinterRedir::CPrinterRedir(Bit16u port, Bit8u irq, const std::string& dev)
    : CParallel(port, irq, dev) {
}

CPrinterRedir::~CPrinterRedir() {
}

bool CPrinterRedir::Putchar(Bit8u val) {
    Write_CON(0xD4);
    Write_PR(val);
    Write_CON(0xD5);
    Write_CON(0xD4);
    Read_SR();
    return true;
}

Bitu CPrinterRedir::Read_PR() {
    //--Added 2011-03-22 by Alun Bestor
    return boxer_PRINTER_readdata(0,1);
    //--End of modifications
}

void CPrinterRedir::Write_PR(Bitu val) {
    //--Added 2011-03-22 by Alun Bestor
    boxer_PRINTER_writedata(0,val,1);
    //--End of modifications
}

Bitu CPrinterRedir::Read_SR() {
    //--Added 2011-03-22 by Alun Bestor
    return boxer_PRINTER_readstatus(0,1);
    //--End of modifications
}

void CPrinterRedir::Write_CON(Bitu val) {
    //--Added 2011-03-22 by Alun Bestor
    boxer_PRINTER_writecontrol(0,val,1);
    //--End of modifications
}

bool CPrinterRedir::IsInited() {
    //--Added 2011-03-22 by Alun Bestor
    return boxer_PRINTER_isInited(0);
    //--End of modifications
}

const char* CPrinterRedir::GetName() {
    //--Added 2011-03-22 by Alun Bestor
    return boxer_PRINTER_getDeviceName(0);
    //--End of modifications
}
```

### printer_redir.h

```cpp
#ifndef DOSBOX_PRINTER_REDIR_H
#define DOSBOX_PRINTER_REDIR_H

#include "parport.h"

class CPrinterRedir final : public CParallel {
public:
    CPrinterRedir(Bit16u port, Bit8u irq, const std::string& dev);
    ~CPrinterRedir() override;

    bool Putchar(Bit8u val) override;
    Bitu Read_PR() override;
    void Write_PR(Bitu val) override;
    Bitu Read_SR() override;
    void Write_CON(Bitu val) override;
    bool IsInited() override;
    const char* GetName() override;
};

#endif
```

### Boxer Callbacks

Defined in `BXCoalface.h`:

```cpp
// Printer port I/O
Bitu boxer_PRINTER_readdata(Bitu port, Bitu iolen);
void boxer_PRINTER_writedata(Bitu port, Bitu val, Bitu iolen);
Bitu boxer_PRINTER_readstatus(Bitu port, Bitu iolen);
void boxer_PRINTER_writecontrol(Bitu port, Bitu val, Bitu iolen);
bool boxer_PRINTER_isInited(Bitu port);
const char* boxer_PRINTER_getDeviceName(Bitu port);
```

Implemented in `BXCoalface.mm`:

```objc
Bitu boxer_PRINTER_readdata(Bitu port, Bitu iolen) {
    BXEmulator *emulator = [BXEmulator currentEmulator];
    return [emulator _readPrinterData:port ioLength:iolen];
}

void boxer_PRINTER_writedata(Bitu port, Bitu val, Bitu iolen) {
    BXEmulator *emulator = [BXEmulator currentEmulator];
    [emulator _writePrinterData:port value:val ioLength:iolen];
}

Bitu boxer_PRINTER_readstatus(Bitu port, Bitu iolen) {
    BXEmulator *emulator = [BXEmulator currentEmulator];
    return [emulator _readPrinterStatus:port ioLength:iolen];
}

void boxer_PRINTER_writecontrol(Bitu port, Bitu val, Bitu iolen) {
    BXEmulator *emulator = [BXEmulator currentEmulator];
    [emulator _writePrinterControl:port value:val ioLength:iolen];
}

bool boxer_PRINTER_isInited(Bitu port) {
    BXEmulator *emulator = [BXEmulator currentEmulator];
    return [emulator _isPrinterInitialized:port];
}

const char* boxer_PRINTER_getDeviceName(Bitu port) {
    BXEmulator *emulator = [BXEmulator currentEmulator];
    return [[emulator _printerDeviceName:port] UTF8String];
}
```

---

## Migration Options

### Option 1: Port Parport Code Forward ⚠️

**Description:** Copy the minimal parport code needed by Boxer into the new DOSBox Staging.

**Files to Port:**
- `parport.cpp/h` (~1,823 lines)
- `printer_redir.cpp/h` (~118 lines)
- `printer_if.h` (~50 lines)

**Total:** ~2,000 lines

**Advantages:**
- ✅ Maintains printer functionality
- ✅ No user-visible changes
- ✅ Proven code, already working

**Disadvantages:**
- ❌ Unmaintained code (removed from upstream)
- ❌ Becomes Boxer's maintenance burden
- ❌ May conflict with future DOSBox changes
- ❌ Increases technical debt
- ❌ No upstream support

**Effort:** Medium (2-3 days)

**Risk:** Medium-High

**Procedure:**

1. Copy files:
```bash
mkdir src/hardware/parport
cp old/src/hardware/parport/parport.{cpp,h} src/hardware/parport/
cp old/src/hardware/parport/printer_redir.{cpp,h} src/hardware/parport/
cp old/src/hardware/parport/printer_if.h src/hardware/parport/
```

2. Update includes in copied files:
```cpp
// In printer_redir.cpp
#include "parport.h"
#include "BXCoalface.h"  // Should work via dosbox.h
```

3. Update build system:
```meson
# In src/hardware/meson.build
if get_option('boxer_integration')
    hardware_sources += files(
        'parport/parport.cpp',
        'parport/printer_redir.cpp',
    )
endif
```

4. Register parallel ports in DOSBox initialization:
```cpp
// In setup code
if (boxer_PRINTER_isInited(0)) {
    new CPrinterRedir(0x378, 7, "Boxer Printer");  // LPT1
}
```

5. Test compilation and runtime

**Recommendation:** ⚠️ **Only if printer usage is confirmed**

---

### Option 2: Stub Implementation ✅

**Description:** Provide no-op implementations of printer callbacks, effectively disabling printer support.

**Files Modified:**
- `BXCoalface.mm` only (no DOSBox changes)

**Advantages:**
- ✅ Minimal effort (1 hour)
- ✅ No DOSBox code changes
- ✅ Clean, maintainable
- ✅ No technical debt

**Disadvantages:**
- ❌ Loses printer functionality
- ❌ User-visible feature loss (if used)

**Effort:** Very Low (1 hour)

**Risk:** Low

**Procedure:**

Edit `Boxer/BXCoalface.mm`:

```objc
// Stub implementations
Bitu boxer_PRINTER_readdata(Bitu port, Bitu iolen) {
    return 0xFF;  // No data available
}

void boxer_PRINTER_writedata(Bitu port, Bitu val, Bitu iolen) {
    // Silently discard output
    // Could optionally log to console if debugging
    // LOG_MSG("Printer write: port=%d, val=0x%02X", port, val);
}

Bitu boxer_PRINTER_readstatus(Bitu port, Bitu iolen) {
    // Return "printer ready" status
    // Bit 7: Not busy
    // Bit 5: Out of paper (0 = has paper)
    // Bit 4: Selected
    // Bit 3: No error
    return 0x90;  // Not busy, selected, no error
}

void boxer_PRINTER_writecontrol(Bitu port, Bitu val, Bitu iolen) {
    // Ignore control writes
}

bool boxer_PRINTER_isInited(Bitu port) {
    return false;  // Printer not available
}

const char* boxer_PRINTER_getDeviceName(Bitu port) {
    return nullptr;
}
```

**No DOSBox changes required** - printer simply won't be available.

**Recommendation:** ✅ **Recommended if printer usage is low/zero**

---

### Option 3: Serial Port Redirection

**Description:** Redirect printer to serial port instead, using DOSBox's existing serial port infrastructure.

**Advantages:**
- ✅ Uses maintained DOSBox code
- ✅ Potentially more flexible

**Disadvantages:**
- ❌ Different configuration for users
- ❌ Not a drop-in replacement
- ❌ Requires significant rework
- ❌ Serial port may be used for other purposes

**Effort:** High (1-2 weeks)

**Risk:** High

**Recommendation:** ❌ **Not recommended** - too much effort for uncertain benefit

---

### Option 4: LPT DAC Only

**Description:** Keep only LPT DAC (parallel port audio) support, drop printing.

**Note:** DOSBox Staging 0.83.0 has `src/hardware/audio/lpt_dac.cpp` for parallel port audio devices (Covox, Disney Sound Source, etc.)

**Advantages:**
- ✅ Maintains audio functionality
- ✅ Uses upstream code
- ✅ No printer complexity

**Disadvantages:**
- ❌ Loses printing
- ❌ May not be what Boxer printer was for

**Effort:** Low (if LPT audio needed)

**Recommendation:** ℹ️ **Consider if audio was the main use case**

---

## Usage Assessment Methodology

Before choosing an option, assess actual printer usage:

### 1. User Surveys

```
Questions for Boxer users:
- Have you ever used printer functionality in Boxer?
- If yes, how frequently?
- What do you print? (text files, graphics, etc.)
- Would losing printer support be a blocker for you?
```

### 2. Analytics (if available)

Check if Boxer collects any usage metrics:
- How many users have configured printers?
- How often is printer I/O performed?
- Any error logs related to printing?

### 3. Support Tickets

Review historical support requests:
- Are there printer-related issues?
- Do users ask about printing?
- Any feature requests for better printing?

### 4. Code Analysis

Check if BXEmulator actually implements printer methods:

```objc
// In Boxer codebase, look for:
@implementation BXEmulator

- (Bitu)_readPrinterData:(Bitu)port ioLength:(Bitu)iolen {
    // If this returns dummy data, printer not really used
}

- (void)_writePrinterData:(Bitu)port value:(Bitu)val ioLength:(Bitu)iolen {
    // If this is a no-op, printer not implemented
}
```

If these methods are stubs or unimplemented, **printer is not actually used** → Choose Option 2 (Stub).

### 5. DOS Software Analysis

Research what DOS software users run in Boxer:
- Are any printer-dependent applications popular?
- Are there workarounds (print to file, then open in macOS)?

---

## Recommendation Decision Tree

```
Do users actually use printing?
├─ No / Unsure
│  └─ Option 2: Stub Implementation ✅
│     - Minimal effort
│     - Clean codebase
│     - Can revisit later if needed
│
├─ Yes, but rarely
│  ├─ Is it critical?
│  │  ├─ Yes → Option 1: Port parport code
│  │  └─ No → Option 2: Stub, with deprecation notice
│  │
│  └─ Can users work around it?
│     ├─ Yes (print to file) → Option 2: Stub
│     └─ No → Option 1: Port parport code
│
└─ Yes, frequently
   └─ Option 1: Port parport code ⚠️
      - Must maintain long-term
      - Document as Boxer-specific
      - Plan for future refactoring
```

---

## Implementation Details for Option 1

If proceeding with porting parport code:

### File Modifications Needed

#### parport.cpp

**Line 1:** Update include guards if needed

**Lines 10-30:** Check for any C++20 incompatibilities

**Lines 50-100:** Port registration - ensure compatible with new DOSBox

**Key functions:**
- `CParallel::CParallel()` - Constructor
- `CParallel::Write_*()` - I/O handlers
- `CParallel::Read_*()` - I/O handlers
- Port registration in initialization

**Changes needed:**
- Update includes for new DOSBox structure
- Verify DOS device registration still works
- Check event system compatibility

#### printer_redir.cpp

**Minimal changes needed:**
- Already imports BXCoalface.h
- All I/O redirected to Boxer
- Should compile as-is

**Verify:**
- BXCoalface.h accessible
- Boxer callbacks linked correctly
- Proper initialization

#### Build System Integration

**Meson:**

Create `src/hardware/parport/meson.build`:

```meson
parport_sources = files(
    'parport.cpp',
    'printer_redir.cpp',
)

libparport = static_library(
    'parport',
    parport_sources,
    include_directories: [inc, include_directories('.')],
    dependencies: [sdl2_dep],
)

libparport_dep = declare_dependency(
    link_with: libparport,
)
```

**Update** `src/hardware/meson.build`:

```meson
if get_option('boxer_integration')
    subdir('parport')
    hardware_deps += libparport_dep
endif
```

### Initialization in DOSBox

Where to register parallel ports? Look in DOSBox initialization code:

```cpp
// In src/hardware/hardware.cpp or similar
void HARDWARE_Init() {
    // ... other hardware init ...

#ifdef BOXER_INTEGRATION
    if (boxer_PRINTER_isInited(0)) {
        // Create Boxer's virtual printer on LPT1
        parallel_devices[0] = new CPrinterRedir(0x378, 7, "Boxer Virtual Printer");
    }
#endif
}
```

### Testing Procedure

1. **Build Test:**
```bash
meson compile -C build
# Should succeed
```

2. **Runtime Test:**
```bash
./build/dosbox
# In DOSBox shell:
DIR > LPT1
# Check if Boxer receives printer data
```

3. **DOS Software Test:**
- Use a DOS program that prints
- Verify output in Boxer
- Check for any errors or crashes

---

## Long-term Maintenance Considerations

### If Keeping Parport Code (Option 1)

**Responsibilities:**
- Boxer team owns this code
- Must fix any bugs
- Must update for C++ standard changes
- Must resolve conflicts with DOSBox updates

**Documentation:**
- Clearly mark as Boxer-specific
- Document why it's needed
- Provide removal instructions if ever deprecated

**Future Migration Path:**
- Consider implementing Boxer's printer entirely in Boxer
- Use file-based communication instead of parport
- Example: DOS prints to file, Boxer monitors file, renders as print

### If Stubbing (Option 2)

**User Communication:**
- Announce printer feature deprecation
- Provide rationale (unmaintained upstream code)
- Offer workarounds (print to file, use PDF printer driver in DOS)

**Potential User Reaction:**
- Most users won't notice (if usage is low)
- Power users may complain
- Be prepared to reconsider if feedback is strong

---

## Recommended Approach

### Proposed Plan

**Phase 1: Research (Week 3, Days 1-2)**
1. Survey Boxer users about printer usage
2. Analyze BXEmulator printer method implementations
3. Review support ticket history
4. Document findings

**Phase 2: Decision (Week 3, Day 3)**
Based on research:
- Low/zero usage → **Option 2 (Stub)**
- Moderate usage → **Option 2 with deprecation notice**
- High usage → **Option 1 (Port code)**

**Phase 3: Implementation (Week 3, Days 4-5)**
- Implement chosen option
- Test thoroughly
- Document decision

**Phase 4: Communication (Week 3, Days 6-7)**
- If removing: Announce to users, provide alternatives
- If keeping: Document maintenance responsibilities
- Update user documentation

---

## Appendix: Boxer Printer Features

### What Boxer's Printer Implementation Provides

(This section would need to be filled in by examining Boxer's actual code)

**Likely features:**
- Virtual printer output to PDF
- Print preview window
- Page formatting
- Font rendering
- Color printing support

**Implementation location:**
Likely in `BXEmulator` or a dedicated printer class.

**To investigate:**
```bash
cd ~/boxer-upgrade/Boxer
grep -r "printer" --include="*.m" --include="*.mm" --include="*.h"
grep -r "PRINTER" --include="*.m" --include="*.mm" --include="*.h"
```

---

## Conclusion

**Default Recommendation:** Start with **Option 2 (Stub)** unless research proves printer is heavily used.

**Rationale:**
1. Lowest risk and effort
2. Clean codebase (no unmaintained code)
3. Can always add back later if needed
4. Most users likely don't use printing

**If printer proves essential:** Fall back to **Option 1**, with clear documentation that this is Boxer-maintained code.

---

**Document Version:** 1.0
**Date:** 2025-11-14
**Status:** Recommendation pending user research
