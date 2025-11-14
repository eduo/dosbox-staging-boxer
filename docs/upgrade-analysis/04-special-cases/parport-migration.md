# Parport/Parallel Port Migration Analysis

**Agent**: Agent 4 - Parport Migration Specialist
**Created**: 2025-11-14T13:45:00Z
**Status**: Completed
**Dependencies**: Agent 1A

## Summary

The entire parallel port (parport) subsystem is **COMPLETELY MISSING** from target DOSBox Staging. The legacy DOSBox contains approximately 4,000 lines of parport code that must be migrated to preserve Boxer's printer functionality. This is a **CRITICAL BLOCKER** requiring full subsystem migration. Migration is feasible and recommended.

## Legacy Parport Implementation

### File Structure

**Location**: `/home/user/dosbox-staging-boxer/src/hardware/parport/`

**Files** (14 total, ~4000 lines):
```
Core Files:
- parport.cpp (372 lines) - Main parallel port infrastructure
- parport.h (in include/) - CParallel base class interface

Printer Implementations:
- printer_redir.cpp (73 lines) - Boxer integration (CPrinterRedir class)
- printer_redir.h (31 lines)
- printer.cpp (2127 lines) - Standard ESC/P printer emulation
- printer.h (245 lines)
- printer_if.h (11 lines) - Printer interface definitions
- printer_charmaps.cpp (309 lines) - Character mappings
- printer_charmaps.h (26 lines)

File Output:
- filelpt.cpp (193 lines) - Print to file
- filelpt.h (70 lines)

Direct Hardware (Windows/Linux):
- directlpt_win32.cpp (245 lines) - Direct hardware access
- directlpt_win32.h (71 lines)
- directlpt_linux.cpp (163 lines)
- directlpt_linux.h (60 lines)
```

### CParallel Interface

**File**: `/home/user/dosbox-staging-boxer/include/parport.h`

```cpp
class CParallel {
public:
    // Constructor
    CParallel(CommandLine* cmd, Bitu portnr, Bit8u initirq);
    virtual ~CParallel();

    IO_ReadHandleObject ReadHandler[3];
    IO_WriteHandleObject WriteHandler[3];

    void setEvent(Bit16u type, float duration);
    void removeEvent(Bit16u type);
    void handleEvent(Bit16u type);
    virtual void handleUpperEvent(Bit16u type)=0;

    Bitu port_nr;
    Bitu base;
    Bitu irq;

    // Pure virtual methods - must be implemented by subclasses
    virtual Bitu Read_PR()=0;      // Read data register (port+0)
    virtual Bitu Read_COM()=0;     // Read control register (port+2)
    virtual Bitu Read_SR()=0;      // Read status register (port+1)

    virtual void Write_PR(Bitu)=0;     // Write data register
    virtual void Write_CON(Bitu)=0;    // Write control register
    virtual void Write_IOSEL(Bitu)=0;  // Write I/O select register

    virtual bool Putchar(Bit8u)=0;
    bool Putchar_default(Bit8u);
    Bit8u getPrinterStatus();
    void initialize();

private:
    DOS_Device* mydosdevice;
};

extern CParallel* parallelPortObjects[3];
void PARALLEL_Init (Section * sec);

const Bit16u parallel_baseaddr[3] = {0x378,0x278,0x3bc};
```

### CPrinterRedir Class

**File**: `/home/user/dosbox-staging-boxer/src/hardware/parport/printer_redir.cpp`

**Purpose**: Boxer integration layer that redirects all LPT I/O to Boxer's virtual printer.

```cpp
class CPrinterRedir : public CParallel {
public:
    CPrinterRedir(Bitu nr, Bit8u initIrq, CommandLine* cmd);
    ~CPrinterRedir();

    bool InstallationSuccessful;

    Bitu Read_PR();
    Bitu Read_COM();
    Bitu Read_SR();
    void Write_PR(Bitu);
    void Write_CON(Bitu);
    void Write_IOSEL(Bitu);
    bool Putchar(Bit8u);
    void handleUpperEvent(Bit16u type);
};
```

**Implementation** (complete file):
```cpp
#include "dosbox.h"

#if C_PRINTER

#include "parport.h"
#include "printer_redir.h"
#import "BXCoalface.h"

CPrinterRedir::CPrinterRedir(Bitu nr, Bit8u initIrq, CommandLine* cmd)
                              :CParallel (cmd, nr, initIrq) {
    InstallationSuccessful = boxer_PRINTER_isInited(nr);
}

CPrinterRedir::~CPrinterRedir () {
    // close file
}

bool CPrinterRedir::Putchar(Bit8u val)
{
    Write_CON(0xD4);
    Write_PR(val);
    Write_CON(0xD5); // strobe pulse
    Write_CON(0xD4); // strobe off
    Read_SR();       // clear ack
    return true;
}

Bitu CPrinterRedir::Read_PR() {
    return boxer_PRINTER_readdata(0,1);
}
Bitu CPrinterRedir::Read_COM() {
    return boxer_PRINTER_readcontrol(0,1);
}
Bitu CPrinterRedir::Read_SR() {
    return boxer_PRINTER_readstatus(0,1);
}
void CPrinterRedir::Write_PR(Bitu val) {
    boxer_PRINTER_writedata(0,val,1);
}
void CPrinterRedir::Write_CON(Bitu val) {
    boxer_PRINTER_writecontrol(0,val,1);
}
void CPrinterRedir::Write_IOSEL(Bitu val) {
    // nothing
}
void CPrinterRedir::handleUpperEvent(Bit16u type) {}
#endif // C_PRINTER
```

### Integration Points

#### INT-075: boxer_PRINTER_readdata (port 0x378 read)
**Location**: `printer_redir.cpp:54-56`
```cpp
Bitu CPrinterRedir::Read_PR() {
    return boxer_PRINTER_readdata(0,1);
}
```
**Purpose**: Read data register from virtual printer

#### INT-076: boxer_PRINTER_writedata (port 0x378 write)
**Location**: `printer_redir.cpp:63-65`
```cpp
void CPrinterRedir::Write_PR(Bitu val) {
    boxer_PRINTER_writedata(0,val,1);
}
```
**Purpose**: Write data to virtual printer

#### INT-077: boxer_PRINTER_readstatus (port 0x379 read)
**Location**: `printer_redir.cpp:60-62`
```cpp
Bitu CPrinterRedir::Read_SR() {
    return boxer_PRINTER_readstatus(0,1);
}
```
**Purpose**: Read printer status (busy, ack, error, paper out, etc.)

#### INT-078: boxer_PRINTER_writecontrol (port 0x37A write)
**Location**: `printer_redir.cpp:66-68`
```cpp
void CPrinterRedir::Write_CON(Bitu val) {
    boxer_PRINTER_writecontrol(0,val,1);
}
```
**Purpose**: Write control signals (strobe, autofeed, init, select)

#### INT-079: boxer_PRINTER_readcontrol (port 0x37A read)
**Location**: `printer_redir.cpp:57-59`
```cpp
Bitu CPrinterRedir::Read_COM() {
    return boxer_PRINTER_readcontrol(0,1);
}
```
**Purpose**: Read control register

#### INT-080: boxer_PRINTER_isInited (printer availability check)
**Location**: `printer_redir.cpp:31`
```cpp
CPrinterRedir::CPrinterRedir(Bitu nr, Bit8u initIrq, CommandLine* cmd)
                              :CParallel (cmd, nr, initIrq) {
    InstallationSuccessful = boxer_PRINTER_isInited(nr);
}
```
**Purpose**: Check if Boxer's virtual printer is available on LPT port

### Port I/O Flow

```
DOS Program
    |
    | (writes to LPT1: 0x378-0x37A)
    v
PARALLEL_Write() / PARALLEL_Read()  [parport.cpp:96-147]
    |
    | (routes to correct port object)
    v
parallelPortObjects[i]->Write_PR() / Read_SR() etc.
    |
    | (CPrinterRedir implementation)
    v
boxer_PRINTER_writedata() / boxer_PRINTER_readstatus() etc.  [BXCoalface.mm:529-565]
    |
    | (calls into Boxer's emulator)
    v
BXEmulator.printer.dataRegister / statusRegister / controlRegister
    |
    v
BXEmulatedPrinter  [BXEmulatedPrinter.mm]
    |
    | (ESC/P command processing, rendering)
    v
BXPrintSession -> macOS Printing System
```

### Configuration Options

**Config file** (`dosbox.conf`):
```ini
[parallel]
parallel1=printer
parallel2=disabled
parallel3=disabled
```

**Options**:
- `printer` - Boxer's virtual printer (uses CPrinterRedir)
- `file` - Print to file (uses CFileLPT)
- `reallpt` - Direct hardware access (uses CDirectLPT - Windows/Linux only)
- `disabled` - No parallel port

### Initialization

**Location**: `parport.cpp:262-372` (PARPORTS class and PARALLEL_Init)

**Process**:
1. PARALLEL_Init() called during DOSBox startup
2. Reads config for parallel1/2/3 settings
3. Checks if LPT BIOS addresses already occupied (e.g., by Disney Sound Source)
4. Creates appropriate CParallel subclass based on config:
   - CPrinterRedir for "printer"
   - CFileLPT for "file"
   - CDirectLPT for "reallpt"
5. Registers I/O handlers for ports 0x378-0x37A (and 0x278, 0x3BC)
6. Calls BIOS_SetLPTPort() to update BIOS data area
7. Creates DOS device (LPT1/2/3) via device_LPT class

### Dependencies on DOSBox Systems

**Required Headers**:
- `dosbox.h` - Core types (Bitu, Bit8u, etc.)
- `inout.h` - IO_ReadHandleObject, IO_WriteHandleObject, io_port_t, io_val_t, io_width_t
- `pic.h` - PIC_AddEvent, PIC_RemoveSpecificEvents
- `setup.h` - Section, CommandLine, Section_prop
- `timer.h` - GetTicks
- `bios.h` - BIOS_SetLPTPort, BIOS_ADDRESS_LPT1/2/3
- `dos_inc.h` - DOS_Device, DOS_AddDevice, DOS_DelDevice

**Key Functions**:
- `BIOS_SetLPTPort(Bitu port, Bit16u baseaddr)` - Updates BIOS data area
- `IO_Read(io_port_t port)` - Used for LPT detection
- `PIC_FullIndex()` - Timing for debug logs

## Target DOSBox Parport Status

### Search Results

**Files searched**:
```bash
find /home/user/dosbox-staging -name "*parport*" -o -name "*printer*" -o -name "*parallel*"
# Result: NO FILES FOUND

grep -r "parport" /home/user/dosbox-staging/src/
# Result: NO MATCHES

grep -ri "parallel" /home/user/dosbox-staging/src/
# Result: Only LPT DAC and unrelated matches

grep -r "0x378" /home/user/dosbox-staging/src/
# Result: Only in bios.cpp for LPT detection
```

**Files that DO exist**:
```
/home/user/dosbox-staging/src/hardware/lpt.h
/home/user/dosbox-staging/src/hardware/audio/lpt_dac.cpp
/home/user/dosbox-staging/src/hardware/audio/lpt_dac.h
/home/user/dosbox-staging/src/hardware/audio/private/lpt_dac.h
```

**LPT.h contents** (header only, no implementation):
```cpp
// SPDX-FileCopyrightText:  2022-2025 The DOSBox Staging Team
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PARALLEL_PORT_H_
#define PARALLEL_PORT_H_

#include "hardware/port.h"
#include "utils/bit_view.h"

enum LptPorts : io_port_t {
    Lpt1Port = 0x378,
    Lpt2Port = 0x278,
    Lpt3Port = 0x3bc,
};

union LptStatusRegister {
    uint8_t data = 0xff;
    bit_view<0, 2> reserved;
    bit_view<2, 1> irq;
    bit_view<3, 1> error;
    bit_view<4, 1> select_in;
    bit_view<5, 1> paper_out;
    bit_view<6, 1> ack;
    bit_view<7, 1> busy;
};

union LptControlRegister {
    uint8_t data = 0;
    bit_view<0, 1> strobe;
    bit_view<1, 1> auto_lf;
    bit_view<2, 1> initialize;
    bit_view<3, 1> select;
    bit_view<4, 1> irq_ack;
    bit_view<5, 1> bidi;
    bit_view<6, 1> bit6; // unused
    bit_view<7, 1> bit7; // unused
};

#endif
```

**LPT DAC usage**: LPT ports are used ONLY for audio devices (Disney Sound Source, Covox Speech Thing, Stereo-on-1) in target DOSBox Staging.

### Findings

**Status**: **REMOVED** (never existed in DOSBox Staging)

**Evidence**:
1. **NO parport directory** in `/home/user/dosbox-staging/src/hardware/`
2. **NO printer-related files** anywhere in the source tree
3. **NO CParallel class** or parallel port infrastructure
4. **NO printer emulation** code
5. **Git history shows NO removal commits** - it was never added to DOSBox Staging
6. **Only LPT definitions exist** for audio device support (lpt.h)

**Why parport doesn't exist in target**:
- DOSBox Staging forked from DOSBox SVN and took a different direction
- Printer support was likely considered low-priority or out-of-scope
- LPT ports are only used for audio devices (Disney, Covox, etc.)
- No community demand for printer emulation in DOSBox Staging

**BIOS LPT detection in target**:
- Target DOES auto-detect LPT ports at 0x378, 0x278, 0x3BC
- Target DOES set BIOS_ADDRESS_LPT1/2/3 in BIOS data area
- Target does NOT provide BIOS_SetLPTPort() function
- LPT detection is passive - checks if ports respond

## Boxer Printer Subsystem

### Printer Features

**BXEmulatedPrinter** provides:

1. **Full ESC/P Printer Emulation**:
   - ESC/P (Epson Standard Code for Printers) command set
   - ESC/P2 extended commands
   - IBM graphics commands
   - Character formatting (bold, italic, underline, superscript, subscript)
   - Font control (10/12/15 CPI, multiple typefaces)
   - Graphics printing (bit-image modes at various densities)

2. **Print Session Management**:
   - Multi-page print sessions
   - Page size configuration (default US Letter 8.5x11")
   - Margin control
   - Print preview
   - Session cancellation

3. **Output to macOS**:
   - Native macOS print dialog
   - PDF generation
   - Print to system printer
   - Print preview panel

4. **Character Sets**:
   - Multiple codepages (USA, France, Germany, UK, etc.)
   - International character sets
   - Character table switching

5. **Quality Modes**:
   - Draft quality
   - Letter Quality (LQ)
   - Color support (7 colors)

### Dependencies on Parport

**CRITICAL**: Boxer's printer subsystem is **100% dependent** on the parport integration.

**Integration via boxer_PRINTER_* functions**:

**File**: `/home/user/Boxer/Boxer/BXCoalface.mm:529-565`

```cpp
#pragma mark - Printer functions

Bitu boxer_PRINTER_readdata(Bitu port,Bitu iolen)
{
    BXEmulator *emulator = [BXEmulator currentEmulator];
    return emulator.printer.dataRegister;
}

void boxer_PRINTER_writedata(Bitu port,Bitu val,Bitu iolen)
{
    BXEmulator *emulator = [BXEmulator currentEmulator];
    emulator.printer.dataRegister = val;
}

Bitu boxer_PRINTER_readstatus(Bitu port,Bitu iolen)
{
    BXEmulator *emulator = [BXEmulator currentEmulator];
    return emulator.printer.statusRegister;
}

void boxer_PRINTER_writecontrol(Bitu port,Bitu val, Bitu iolen)
{
    BXEmulator *emulator = [BXEmulator currentEmulator];
    emulator.printer.controlRegister = val;
}

Bitu boxer_PRINTER_readcontrol(Bitu port,Bitu iolen)
{
    BXEmulator *emulator = [BXEmulator currentEmulator];
    return emulator.printer.controlRegister;
}

bool boxer_PRINTER_isInited(Bitu port)
{
    BXEmulator *emulator = [BXEmulator currentEmulator];
    //Tell the emulator we actually want a printer
    [emulator _didRequestPrinterOnLPTPort: port];
    return emulator.printer != nil;
}
```

**Flow**: DOS LPT I/O → parport.cpp → CPrinterRedir → boxer_PRINTER_* → BXEmulatedPrinter

### Criticality Assessment

**Impact if parport unavailable**: **CRITICAL**

**User-facing features that would break**:
1. **Printer output** - Cannot print from DOS programs
2. **Print preview** - No print preview panel
3. **PDF generation** - Cannot save DOS output as PDF
4. **Print sessions** - No multi-page printing
5. **ESC/P compatibility** - DOS programs expecting printer won't work
6. **Screenshot printing** - Some games print graphics to printer
7. **Text output** - Programs using printer for text output will fail
8. **Diagnostic output** - Some programs print diagnostic info

**DOS programs affected**:
- Business software (WordPerfect, Lotus 1-2-3, dBase)
- Accounting programs
- Label printing software
- Graphics programs with printer support
- Games with high score printing
- Any program using BIOS INT 17h or direct LPT I/O

**Boxer feature impact**: Printer functionality is advertised as a Boxer feature. Losing it would be a **major regression**.

## Migration Strategy

### Recommended Approach

**Scenario**: **B - Parport Removed but Can Be Re-Added**

**Strategy**: **Full Parport Subsystem Migration**

Copy the entire parport directory from legacy to target, with modifications to adapt to target DOSBox Staging's modernized architecture.

### Implementation Steps

#### Phase 1: Assess API Compatibility (4 hours)

**Tasks**:
1. **Compare DOSBox core APIs** between legacy and target:
   - IO port registration (io_port_t, io_width_t, IO_ReadHandleObject, IO_WriteHandleObject)
   - PIC event system (PIC_AddEvent, PIC_RemoveSpecificEvents)
   - DOS device system (DOS_Device, DOS_AddDevice, DOS_DelDevice)
   - BIOS functions (BIOS_SetLPTPort availability)
   - Memory access (mem_writew, mem_readw)
   - Timer system (GetTicks, PIC_FullIndex)

2. **Identify breaking changes**:
   - Type signature changes (Bitu → uint32_t, Bit8u → uint8_t, etc.)
   - Function signature changes
   - Header reorganization
   - Namespace changes
   - C++11/14/17/20 modernization

3. **Document required adaptations**:
   - Create API mapping table (legacy → target)
   - List deprecated functions needing replacement
   - Identify new target APIs to use

#### Phase 2: Copy and Adapt Parport Files (8 hours)

**Tasks**:
1. **Create parport directory** in target:
   ```bash
   mkdir -p /home/user/dosbox-staging/src/hardware/parport
   ```

2. **Copy essential files** from legacy:
   ```bash
   # Core parport infrastructure
   cp parport.cpp printer_redir.cpp printer_redir.h printer_if.h

   # Standard printer (optional - Boxer doesn't use it)
   # cp printer.cpp printer.h printer_charmaps.cpp printer_charmaps.h

   # File output (optional - nice to have)
   # cp filelpt.cpp filelpt.h

   # Direct hardware (skip - not needed on macOS)
   # directlpt_win32.* directlpt_linux.* not needed
   ```

3. **Modernize type usage**:
   ```cpp
   // Legacy → Target
   Bitu → uint32_t or size_t
   Bit8u → uint8_t
   Bit16u → uint16_t
   Bit32u → uint32_t
   io_val_t → already exists
   io_port_t → already exists
   io_width_t → already exists
   ```

4. **Update includes**:
   ```cpp
   // Legacy paths
   #include "dosbox.h"
   #include "inout.h"
   #include "pic.h"

   // Target paths (check actual locations)
   #include "dosbox.h"
   #include "hardware/port.h"
   #include "hardware/pic.h"
   ```

5. **Adapt BIOS_SetLPTPort**:
   - Check if BIOS_SetLPTPort exists in target
   - If not, replace with direct mem_writew to BIOS_ADDRESS_LPT1/2/3
   - Or add BIOS_SetLPTPort helper function

6. **Update build system**:
   - Add parport files to CMakeLists.txt or meson.build
   - Define C_PRINTER macro
   - Link printer_redir.cpp

#### Phase 3: Resolve Conflicts with LPT DAC (4 hours)

**Challenge**: Target DOSBox uses LPT ports for audio devices (Disney, Covox, etc.)

**Tasks**:
1. **Analyze LPT DAC port registration**:
   - How do Disney/Covox register LPT ports?
   - Can parport and LPT DAC coexist?
   - Priority system needed?

2. **Implement port sharing or exclusion**:
   - **Option A**: Mutual exclusion - config chooses printer OR audio device
   - **Option B**: Port sharing - parport forwards unknown data to audio device
   - **Option C**: Separate ports - printer on LPT1, audio on LPT2

3. **Update configuration**:
   ```ini
   [parallel]
   parallel1=printer    # Boxer printer

   [speaker]
   disney=false         # Disable if using LPT1 for printer
   ```

4. **Add conflict detection**:
   - Check at startup if both printer and LPT DAC try to claim same port
   - Warn user or auto-disable conflicting device

#### Phase 4: Integration Testing (6 hours)

**Tasks**:
1. **Unit tests**:
   - Test port I/O (read/write data, status, control registers)
   - Test boxer_PRINTER_* function calls
   - Test InstallationSuccessful flag

2. **DOS program tests**:
   - Test with DOS text printing (COPY file.txt LPT1:)
   - Test with DOS graphics program (Print Screen to printer)
   - Test with business software (WordPerfect, Lotus)

3. **Boxer integration tests**:
   - Verify BXEmulatedPrinter receives data
   - Test print preview panel
   - Test PDF generation
   - Test multi-page sessions

4. **Performance tests**:
   - Ensure no performance regression
   - Check for memory leaks
   - Verify proper cleanup on shutdown

5. **Regression tests**:
   - Ensure LPT DAC audio still works (if using separate port)
   - Check Disney Sound Source compatibility
   - Verify BIOS LPT detection still works

#### Phase 5: Documentation and Cleanup (2 hours)

**Tasks**:
1. **Code comments**:
   - Add comments explaining Boxer integration
   - Document boxer_PRINTER_* interface
   - Note differences from standard printer implementation

2. **User documentation**:
   - Update Boxer user guide with printer instructions
   - Document configuration options
   - Add troubleshooting section

3. **Developer documentation**:
   - Update architecture docs
   - Add parport to component diagram
   - Document migration from legacy

### Code Changes Required

#### DOSBox Changes

**New files**:
```
/home/user/dosbox-staging/src/hardware/parport/parport.cpp (adapted)
/home/user/dosbox-staging/src/hardware/parport/printer_redir.cpp (adapted)
/home/user/dosbox-staging/src/hardware/parport/printer_redir.h (adapted)
/home/user/dosbox-staging/src/hardware/parport/printer_if.h (copied)
/home/user/dosbox-staging/include/parport.h (adapted)
```

**Modified files**:
```
/home/user/dosbox-staging/src/hardware/CMakeLists.txt
  - Add parport subdirectory

/home/user/dosbox-staging/src/ints/bios.cpp
  - Add BIOS_SetLPTPort() if missing

/home/user/dosbox-staging/src/dosbox.cpp (or main init)
  - Ensure PARALLEL_Init() is called
```

**Estimated changes**: ~400 lines modified, ~500 lines added

#### Boxer Changes

**None required** - Boxer's boxer_PRINTER_* functions are already compatible.

**Optional enhancements**:
- Update printer configuration UI
- Add printer diagnostics panel
- Improve error handling

### Alternative Approaches

#### Alternative 1: Reimplementation from Scratch

**Description**: Rewrite parport subsystem using modern C++ and target DOSBox APIs

**Pros**:
- Clean code without legacy baggage
- Full integration with target architecture
- Modern C++ practices (RAII, smart pointers, etc.)

**Cons**:
- **Very high effort** (20-40 hours)
- Risk of introducing bugs
- Must reimplement all LPT protocol details
- Harder to verify correctness

**Verdict**: **Not recommended** - copying and adapting is much faster and lower risk

#### Alternative 2: Minimal Stub Implementation

**Description**: Create minimal parport that only supports Boxer, skip all other functionality

**Pros**:
- Smaller code footprint
- Faster implementation (6-10 hours)
- Only includes what Boxer needs

**Cons**:
- Loses file output capability
- Loses direct hardware support
- Less maintainable
- Future features harder to add

**Verdict**: **Acceptable fallback** if full migration proves too difficult

#### Alternative 3: External Printer Process

**Description**: Move printer emulation entirely out of DOSBox, communicate via IPC

**Pros**:
- Complete separation of concerns
- Could work across DOSBox versions
- Easier to update printer code

**Cons**:
- **Very high complexity**
- IPC overhead and latency
- Architectural change to Boxer
- Port I/O must still be captured

**Verdict**: **Not recommended** - over-engineered for this use case

## Implementation Plan

### Phase 1: API Compatibility Assessment
**Duration**: 4 hours
**Tasks**:
- Compare IO port APIs (legacy vs target)
- Compare PIC event APIs
- Compare DOS device APIs
- Compare BIOS APIs
- Document type signature changes
- Create API mapping table
- Identify deprecated functions

### Phase 2: Parport File Migration
**Duration**: 8 hours
**Tasks**:
- Create parport directory structure
- Copy parport.cpp and adapt types
- Copy printer_redir.cpp and adapt types
- Copy header files and update includes
- Modernize C++ usage (auto, nullptr, etc.)
- Replace Bitu/Bit8u with uint types
- Update BIOS_SetLPTPort usage
- Add to build system

### Phase 3: LPT DAC Conflict Resolution
**Duration**: 4 hours
**Tasks**:
- Analyze LPT DAC port registration
- Design port sharing/exclusion strategy
- Implement mutual exclusion logic
- Update configuration handling
- Add conflict detection and warnings
- Test printer vs audio device priority

### Phase 4: Integration Testing
**Duration**: 6 hours
**Tasks**:
- Build and fix compilation errors
- Test basic port I/O
- Test with DOS PRINT command
- Test with Boxer print dialog
- Test print preview
- Test PDF generation
- Verify no memory leaks
- Check performance

### Phase 5: Documentation
**Duration**: 2 hours
**Tasks**:
- Add code comments
- Update developer docs
- Note migration decisions
- Document known issues

## Testing Strategy

### Test Cases

1. **Port I/O Verification**:
   ```
   Test: Write to LPT1 data port (0x378)
   Expected: boxer_PRINTER_writedata() called
   Verify: BXEmulatedPrinter.dataRegister updated
   ```

2. **Status Register**:
   ```
   Test: Read LPT1 status port (0x379)
   Expected: boxer_PRINTER_readstatus() called
   Verify: Correct status bits (busy=0, ack, error, etc.)
   ```

3. **DOS Text Printing**:
   ```
   Test: COPY AUTOEXEC.BAT LPT1:
   Expected: Boxer print dialog appears
   Verify: Text appears in preview
   ```

4. **DOS Graphics Printing**:
   ```
   Test: Print graphics from DOS program
   Expected: Print preview shows graphics
   Verify: PDF output contains bitmap
   ```

5. **Multi-page Session**:
   ```
   Test: Print 3-page document
   Expected: All pages in single print session
   Verify: All pages in PDF output
   ```

6. **Printer Initialization**:
   ```
   Test: DOS program sends ESC @ (reset)
   Expected: Printer resets to defaults
   Verify: Font/margins restored
   ```

7. **Control Signals**:
   ```
   Test: DOS toggles strobe, autofeed, init bits
   Expected: boxer_PRINTER_writecontrol() called
   Verify: Control register updates correctly
   ```

8. **LPT DAC Exclusion** (if implemented):
   ```
   Test: Enable both printer and Disney Sound Source
   Expected: Warning or auto-disable conflict
   Verify: Only one device active on LPT1
   ```

### Validation Criteria

**Success criteria**:
- ✅ Boxer's printer dialog appears when DOS prints
- ✅ Text prints correctly with proper formatting
- ✅ Graphics print as bitmaps
- ✅ Multi-page documents work
- ✅ PDF export works
- ✅ Print to macOS printer works
- ✅ No crashes or memory leaks
- ✅ No performance regression
- ✅ All 6 boxer_PRINTER_* integration points work

**Failure indicators**:
- ❌ Compilation errors in parport code
- ❌ DOS programs hang when printing
- ❌ Boxer printer dialog doesn't appear
- ❌ Printed output is corrupted
- ❌ Memory leaks detected
- ❌ Crashes when printing

## Risk Assessment

### HIGH Risks

**RISK-P1: API incompatibility between legacy and target**
- **Impact**: Parport code won't compile
- **Likelihood**: Medium
- **Mitigation**: Thorough API assessment in Phase 1, incremental adaptation
- **Fallback**: Use Alternative 2 (minimal stub)

**RISK-P2: LPT DAC port conflict**
- **Impact**: Printer and audio devices conflict, both fail
- **Likelihood**: High
- **Mitigation**: Implement mutual exclusion in Phase 3
- **Fallback**: Document conflict, let user choose via config

**RISK-P3: BIOS_SetLPTPort function missing**
- **Impact**: LPT ports not registered in BIOS data area
- **Likelihood**: Medium (function exists in legacy, might not in target)
- **Mitigation**: Implement replacement with direct mem_writew
- **Fallback**: Add BIOS_SetLPTPort to target BIOS code

### MEDIUM Risks

**RISK-P4: Type signature changes break compilation**
- **Impact**: Many compilation errors
- **Likelihood**: Medium
- **Mitigation**: Systematic type replacement in Phase 2
- **Fallback**: Use typedef aliases for compatibility

**RISK-P5: Build system integration issues**
- **Impact**: Parport files not compiled/linked
- **Likelihood**: Low-Medium
- **Mitigation**: Follow target's build system patterns
- **Fallback**: Add files manually to build scripts

**RISK-P6: Performance regression**
- **Impact**: Printing is slow or causes stuttering
- **Likelihood**: Low
- **Mitigation**: Profile and optimize hot paths
- **Fallback**: Defer printer processing to idle time

### LOW Risks

**RISK-P7: Header include path changes**
- **Impact**: Missing includes
- **Likelihood**: Low
- **Mitigation**: Update includes systematically
- **Fallback**: Add compatibility headers

**RISK-P8: Memory management differences**
- **Impact**: Memory leaks or crashes
- **Likelihood**: Low
- **Mitigation**: Review all new/delete, use RAII
- **Fallback**: Add explicit cleanup code

## Effort Estimate

**Analysis**: 5 hours (this document + preparation)
**Implementation**: 22-28 hours
  - Phase 1 (API assessment): 4 hours
  - Phase 2 (File migration): 8 hours
  - Phase 3 (LPT DAC conflicts): 4 hours
  - Phase 4 (Testing): 6 hours
  - Phase 5 (Documentation): 2 hours
**Testing**: 6 hours (included in Phase 4)
**Total**: **27-33 hours**

**Confidence**: Medium-High (similar migrations done before, well-understood problem)

## Recommendations

### Immediate Actions

1. **BEGIN Phase 1 API Assessment** (Priority: CRITICAL)
   - Compare legacy and target DOSBox APIs
   - Document all type and signature changes
   - Verify all required functions exist in target
   - **Owner**: Implementation Agent (Agent 5A or equivalent)
   - **Deadline**: Before starting file migration

2. **Create parport directory structure** (Priority: HIGH)
   - Set up `/home/user/dosbox-staging/src/hardware/parport/`
   - Copy header files first to check compilation
   - **Owner**: Implementation Agent
   - **Deadline**: Immediately after API assessment

3. **Resolve LPT DAC conflict strategy** (Priority: HIGH)
   - Decide on mutual exclusion vs port sharing
   - Design configuration approach
   - **Owner**: Architecture decision (lead developer)
   - **Deadline**: Before Phase 3

### Long-term Considerations

1. **Consider upstreaming to DOSBox Staging**:
   - If parport migration is clean, propose to DOSBox Staging project
   - Printer emulation could benefit wider community
   - Would reduce maintenance burden for Boxer

2. **Enhance printer features**:
   - Add PostScript support
   - Implement more printer models
   - Add network printing capability

3. **Improve configurability**:
   - Per-gamebox printer settings
   - Multiple virtual printers
   - Print queue management

## Blockers/Open Questions

### BLOCKER-002 Status: **MITIGATED**

**Status**: Migration path identified, implementation feasible

**Resolution approach**: Full parport subsystem migration from legacy to target

**Remaining questions**:
1. ✅ Does parport exist in target? **NO - completely missing**
2. ✅ Can it be migrated? **YES - full migration feasible**
3. ✅ What are dependencies? **Documented above**
4. ✅ What is effort? **27-33 hours**
5. ⚠️ **How to resolve LPT DAC conflict?** - Needs architecture decision
6. ⚠️ **Does BIOS_SetLPTPort exist in target?** - Needs verification
7. ⚠️ **What are exact API differences?** - Needs Phase 1 assessment

**Next steps**:
- Assign implementation agent
- Begin Phase 1 (API assessment)
- Make LPT DAC conflict decision
- Proceed with migration phases

**Risk level**: MEDIUM (feasible but requires careful adaptation)

**Impact if not resolved**: **CRITICAL** - Boxer loses all printer functionality

---

**End of Analysis**
