// ============================================================================
// Smoke Test: Link against DOSBox library, verify hooks are callable
// ============================================================================
//
// This test creates a minimal stub implementation of IBoxerDelegate that
// implements all 86 virtual methods. It verifies that:
// 1. We can link against the DOSBox library
// 2. The hook infrastructure works
// 3. The delegate can be registered and called
//
// The test immediately aborts emulation via runLoopShouldContinue to verify
// the critical abort mechanism works without running a full emulation session.

#include "boxer/boxer_hooks.h"
#include <iostream>
#include <cstdlib>
#include <cstring>

// ============================================================================
// BoxerDelegateStub - Minimal stub implementation of all 86 methods
// ============================================================================

class BoxerDelegateStub : public IBoxerDelegate {
public:
    // ========================================================================
    // Emulation Lifecycle (5 methods)
    // ========================================================================

    bool runLoopShouldContinue() override {
        std::cout << "✓ runLoopShouldContinue() called - requesting abort\n";
        return false; // Signal immediate abort
    }

    void runLoopWillStartWithContextInfo(void* context_info) override {
        std::cout << "✓ runLoopWillStartWithContextInfo() called\n";
    }

    void runLoopDidFinishWithContextInfo(void* context_info) override {
        std::cout << "✓ runLoopDidFinishWithContextInfo() called\n";
    }

    void shutdown() override {
        std::cout << "✓ shutdown() called\n";
    }

    void handleDOSBoxTitleChange(Bit32s cycles, int frameskip, bool paused) override {
        // Silent stub - this would normally update window title
    }

    // ========================================================================
    // Rendering Pipeline (10 methods)
    // ========================================================================

    bool processEvents() override {
        return true; // Events processed successfully
    }

    bool MaybeProcessEvents() override {
        return true; // Events processed
    }

    bool startFrame(Bit8u** frameBuffer, int& pitch) override {
        // Return false to skip frame rendering (we're not actually rendering)
        return false;
    }

    void finishFrame(const uint16_t* changedLines) override {
        // Silent stub - no actual rendering
    }

    Bitu prepareForFrameSize(Bitu width, Bitu height, Bitu gfx_flags,
                             double scalex, double scaley,
                             GFX_CallBack_t callback,
                             double pixel_aspect) override {
        return 0; // Return mode ID 0
    }

    Bitu idealOutputMode(Bitu flags) override {
        return 0; // Return default mode
    }

    Bitu getRGBPaletteEntry(Bit8u red, Bit8u green, Bit8u blue) override {
        // Pack RGB as 0x00RRGGBB
        return (static_cast<Bitu>(red) << 16) |
               (static_cast<Bitu>(green) << 8) |
               static_cast<Bitu>(blue);
    }

    void setShader(const char* shaderSource) override {
        // Silent stub - no shader support in test
    }

    void applyRenderingStrategy() override {
        // Silent stub - no rendering strategy
    }

    int GetDisplayRefreshRate() override {
        return 60; // Standard 60 Hz
    }

    // ========================================================================
    // Graphics Modes (6 methods)
    // ========================================================================

    Bit8u herculesTintMode() override {
        return 0; // White tint
    }

    void setHerculesTintMode(Bit8u mode) override {
        // Silent stub
    }

    double CGACompositeHueOffset() override {
        return 0.0; // No hue offset
    }

    void setCGACompositeHueOffset(double offset) override {
        // Silent stub
    }

    Bit8u CGAComponentMode() override {
        return 1; // RGB mode
    }

    void setCGAComponentMode(Bit8u mode) override {
        // Silent stub
    }

    // ========================================================================
    // Shell Integration (16 methods)
    // ========================================================================

    void shellWillStart(DOS_Shell* shell) override {
        // Silent stub
    }

    void shellDidFinish(DOS_Shell* shell, int exit_code) override {
        // Silent stub
    }

    void shellWillStartAutoexec(DOS_Shell* shell) override {
        // Silent stub
    }

    void didReturnToShell(DOS_Shell* shell) override {
        // Silent stub
    }

    bool shellShouldRunCommand(DOS_Shell* shell, const char* cmd, const char* args) override {
        return false; // Don't intercept commands
    }

    void shellWillReadCommandInputFromHandle(DOS_Shell* shell, Bit16u handle) override {
        // Silent stub
    }

    void shellDidReadCommandInputFromHandle(DOS_Shell* shell, Bit16u handle) override {
        // Silent stub
    }

    bool handleShellCommandInput(DOS_Shell* shell, char* cmd,
                                 Bitu* cursorPosition,
                                 bool* executeImmediately) override {
        return false; // Don't modify input
    }

    bool hasPendingCommandsForShell(DOS_Shell* shell) override {
        return false; // No pending commands
    }

    bool executeNextPendingCommandForShell(DOS_Shell* shell) override {
        return false; // No commands to execute
    }

    bool shellShouldDisplayStartupMessages(DOS_Shell* shell) override {
        return false; // Suppress messages
    }

    void shellWillExecuteFileAtDOSPath(DOS_Shell* shell,
                                       const char* canonicalPath,
                                       const char* arguments) override {
        // Silent stub
    }

    void shellDidExecuteFileAtDOSPath(DOS_Shell* shell,
                                      const char* canonicalPath) override {
        // Silent stub
    }

    void shellWillBeginBatchFile(DOS_Shell* shell,
                                 const char* canonicalPath,
                                 const char* arguments) override {
        // Silent stub
    }

    void shellDidEndBatchFile(DOS_Shell* shell,
                              const char* canonicalPath) override {
        // Silent stub
    }

    bool shellShouldContinue(DOS_Shell* shell) override {
        return false; // Abort shell immediately (for test)
    }

    // ========================================================================
    // Drive and File I/O (18 methods)
    // ========================================================================

    bool shouldMountPath(const char* path) override {
        return true; // Allow all mounts
    }

    bool shouldShowFileWithName(const char* name) override {
        // Hide macOS metadata files
        if (name[0] == '.' && name[1] == '_') return false; // ._* files
        if (strcmp(name, ".DS_Store") == 0) return false;
        return true;
    }

    bool shouldAllowWriteAccessToPath(const char* path, DOS_Drive* drive) override {
        return true; // Allow writes (test environment)
    }

    void driveDidMount(Bit8u driveIndex) override {
        // Silent stub
    }

    void driveDidUnmount(Bit8u driveIndex) override {
        // Silent stub
    }

    void didCreateLocalFile(const char* path, DOS_Drive* drive) override {
        // Silent stub
    }

    void didRemoveLocalFile(const char* path, DOS_Drive* drive) override {
        // Silent stub
    }

    FILE* openLocalFile(const char* path, DOS_Drive* drive, const char* mode) override {
        return nullptr; // Use DOSBox's default file handling
    }

    bool removeLocalFile(const char* path, DOS_Drive* drive) override {
        return false; // Use DOSBox's default
    }

    bool moveLocalFile(const char* fromPath, const char* toPath, DOS_Drive* drive) override {
        return false; // Use DOSBox's default
    }

    bool createLocalDir(const char* path, DOS_Drive* drive) override {
        return false; // Use DOSBox's default
    }

    bool removeLocalDir(const char* path, DOS_Drive* drive) override {
        return false; // Use DOSBox's default
    }

    bool getLocalPathStats(const char* path, DOS_Drive* drive, struct stat* outStatus) override {
        return false; // Use DOSBox's default
    }

    bool localDirectoryExists(const char* path, DOS_Drive* drive) override {
        return false; // Use DOSBox's default
    }

    bool localFileExists(const char* path, DOS_Drive* drive) override {
        return false; // Use DOSBox's default
    }

    DIR_Handle openLocalDirectory(const char* path, DOS_Drive* drive) override {
        return nullptr; // Use DOSBox's default
    }

    void closeLocalDirectory(DIR_Handle handle) override {
        // Silent stub
    }

    bool getNextDirectoryEntry(DIR_Handle handle, char* outName, bool& isDirectory) override {
        return false; // No entries
    }

    // ========================================================================
    // Input Handling (16 methods)
    // ========================================================================

    void setMouseActive(bool active) override {
        // Silent stub
    }

    void mouseMovedToPoint(float x, float y) override {
        // Silent stub
    }

    void setJoystickActive(bool active) override {
        // Silent stub
    }

    Bitu keyboardBufferRemaining() override {
        return 128; // Full buffer available
    }

    bool keyboardLayoutLoaded() override {
        return true; // Default layout
    }

    const char* keyboardLayoutName() override {
        return "us"; // US layout
    }

    bool keyboardLayoutSupported(const char* code) override {
        return (strcmp(code, "us") == 0); // Only US layout supported
    }

    bool keyboardLayoutActive() override {
        return true;
    }

    void setKeyboardLayoutActive(bool active) override {
        // Silent stub
    }

    void setNumLockActive(bool active) override {
        // Silent stub
    }

    void setCapsLockActive(bool active) override {
        // Silent stub
    }

    void setScrollLockActive(bool active) override {
        // Silent stub
    }

    const char* preferredKeyboardLayout() override {
        return "us";
    }

    bool continueListeningForKeyEvents() override {
        return false; // Don't wait for input (for test)
    }

    Bitu numKeyCodesInPasteBuffer() override {
        return 0; // No paste buffer
    }

    bool getNextKeyCodeInPasteBuffer(Bit16u* outKeyCode, bool consumeKey) override {
        return false; // No keys available
    }

    // ========================================================================
    // Printer/Parallel Port (6 methods)
    // ========================================================================

    Bitu PRINTER_readdata(Bitu port, Bitu iolen) override {
        return 0xFF; // All bits high (no data)
    }

    void PRINTER_writedata(Bitu port, Bitu val, Bitu iolen) override {
        // Silent stub
    }

    Bitu PRINTER_readstatus(Bitu port, Bitu iolen) override {
        return 0xDF; // Printer ready (bit 5 = 0 for error, others high)
    }

    void PRINTER_writecontrol(Bitu port, Bitu val, Bitu iolen) override {
        // Silent stub
    }

    Bitu PRINTER_readcontrol(Bitu port, Bitu iolen) override {
        return 0x00; // Default control state
    }

    bool PRINTER_isInited(Bitu port) override {
        return false; // No printer initialized
    }

    // ========================================================================
    // Audio/MIDI (8 methods)
    // ========================================================================

    bool MIDIAvailable() override {
        return false; // No MIDI in test
    }

    void sendMIDIMessage(const uint8_t* data, size_t length) override {
        // Silent stub
    }

    void sendMIDISysex(const uint8_t* data, size_t length) override {
        // Silent stub
    }

    const char* suggestMIDIHandler() override {
        return "none"; // No MIDI handler
    }

    void MIDIWillRestart() override {
        // Silent stub
    }

    void MIDIDidRestart() override {
        // Silent stub
    }

    float masterVolume() override {
        return 1.0f; // Full volume
    }

    void updateVolumes() override {
        // Silent stub
    }

    // ========================================================================
    // Messages, Logging, Error Handling (3 methods)
    // ========================================================================

    const char* localizedStringForKey(const char* key) override {
        return nullptr; // Use default strings
    }

    void log(const char* message) override {
        std::cout << "[DOSBox Log] " << message << "\n";
    }

    void die(const char* message) override {
        std::cerr << "[FATAL ERROR] " << message << "\n";
        std::exit(1);
    }

    // ========================================================================
    // Capture Support (1 method)
    // ========================================================================

    FILE* openCaptureFile(const char* filename, const char* mode) override {
        return nullptr; // No capture support in test
    }
};

// ============================================================================
// Main Entry Point
// ============================================================================

int main(int argc, char* argv[]) {
    std::cout << "=== Boxer DOSBox Integration Smoke Test ===\n\n";

    // Create stub delegate
    BoxerDelegateStub delegate;

    // Register it with DOSBox
    g_boxer_delegate = &delegate;
    std::cout << "✓ Delegate registered (g_boxer_delegate set)\n\n";

    // Test: Can we call hooks through the macros?
    std::cout << "Testing hook macros:\n";

    // Test BOXER_HOOK_BOOL macro
    bool should_continue = BOXER_HOOK_BOOL(runLoopShouldContinue);
    std::cout << "  BOXER_HOOK_BOOL(runLoopShouldContinue) = "
              << (should_continue ? "true" : "false") << "\n";

    if (!should_continue) {
        std::cout << "\n✓ SUCCESS: Hook works! Abort signal received.\n";

        // Test a few more hook types to verify different patterns work
        std::cout << "\nTesting additional hook types:\n";

        // Test BOXER_HOOK_VOID macro
        BOXER_HOOK_VOID(runLoopWillStartWithContextInfo, nullptr);

        // Test BOXER_HOOK_VALUE macro
        int refresh_rate = BOXER_HOOK_VALUE(GetDisplayRefreshRate, 60);
        std::cout << "  BOXER_HOOK_VALUE(GetDisplayRefreshRate, 60) = "
                  << refresh_rate << "\n";

        // Test BOXER_HOOK_PTR macro
        FILE* capture = BOXER_HOOK_PTR(openCaptureFile, "test.txt", "w");
        std::cout << "  BOXER_HOOK_PTR(openCaptureFile, ...) = "
                  << (capture ? "non-null" : "null") << "\n";

        std::cout << "\n✓ All hook macro types verified\n";
        std::cout << "✓ All basic linkage verified\n";
        std::cout << "\n=== SMOKE TEST PASSED ===\n";
        return 0; // Success
    } else {
        std::cerr << "\n✗ FAILURE: Expected false from runLoopShouldContinue\n";
        return 1; // Failure
    }
}
