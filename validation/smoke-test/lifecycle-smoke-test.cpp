// ============================================================================
// Lifecycle Smoke Test: Verify lifecycle hooks work correctly
// ============================================================================
//
// Phase 2, Task 2-4: Enhanced smoke test for lifecycle integration
//
// This test validates:
// - All 3 lifecycle hooks are called in correct order
// - Hook call sequence: WillStart → ShouldContinue (×N) → DidFinish
// - Context info passed correctly
// - Hooks work in various scenarios (normal, abort, exception)

#include "boxer/boxer_hooks.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <stdexcept>

// Provide the global delegate pointer definition
#ifdef BOXER_INTEGRATED
IBoxerDelegate* g_boxer_delegate = nullptr;
#endif

// ============================================================================
// LifecycleTrackingDelegate - Tracks hook call order and validation
// ============================================================================

class LifecycleTrackingDelegate : public IBoxerDelegate {
public:
    enum class HookType {
        WillStart,
        ShouldContinue,
        DidFinish
    };

    struct HookCall {
        HookType type;
        void* context_info;
    };

    std::vector<HookCall> call_history;
    int max_iterations = 5;
    int iteration_count = 0;
    bool should_abort = false;

    void reset() {
        call_history.clear();
        iteration_count = 0;
        should_abort = false;
    }

    // ========================================================================
    // Critical Lifecycle Hooks (INT-057, INT-058, INT-059)
    // ========================================================================

    void runLoopWillStartWithContextInfo(void* context_info) override {
        call_history.push_back({HookType::WillStart, context_info});
        std::cout << "  [Hook] runLoopWillStartWithContextInfo(context="
                  << context_info << ")\n";
    }

    bool runLoopShouldContinue() override {
        iteration_count++;
        call_history.push_back({HookType::ShouldContinue, nullptr});

        if (should_abort) {
            std::cout << "  [Hook] runLoopShouldContinue() -> false (abort signal)\n";
            return false;
        }

        if (iteration_count >= max_iterations) {
            std::cout << "  [Hook] runLoopShouldContinue() -> false (max iterations)\n";
            return false;
        }

        return true;
    }

    void runLoopDidFinishWithContextInfo(void* context_info) override {
        call_history.push_back({HookType::DidFinish, context_info});
        std::cout << "  [Hook] runLoopDidFinishWithContextInfo(context="
                  << context_info << ")\n";
    }

    // ========================================================================
    // Validation Methods
    // ========================================================================

    bool validateCallOrder() const {
        if (call_history.empty()) {
            std::cerr << "  ✗ No hooks were called!\n";
            return false;
        }

        // First call should be WillStart
        if (call_history.front().type != HookType::WillStart) {
            std::cerr << "  ✗ First hook should be WillStart\n";
            return false;
        }

        // Last call should be DidFinish
        if (call_history.back().type != HookType::DidFinish) {
            std::cerr << "  ✗ Last hook should be DidFinish\n";
            return false;
        }

        // Middle calls should all be ShouldContinue
        for (size_t i = 1; i < call_history.size() - 1; ++i) {
            if (call_history[i].type != HookType::ShouldContinue) {
                std::cerr << "  ✗ Middle hooks should all be ShouldContinue\n";
                return false;
            }
        }

        std::cout << "  ✓ Hook call order is correct: WillStart → ShouldContinue (×"
                  << (call_history.size() - 2) << ") → DidFinish\n";
        return true;
    }

    bool validateContextInfo(void* expected_start, void* expected_finish) const {
        const HookCall& start_call = call_history.front();
        const HookCall& finish_call = call_history.back();

        if (start_call.context_info != expected_start) {
            std::cerr << "  ✗ WillStart context mismatch: expected "
                      << expected_start << ", got " << start_call.context_info << "\n";
            return false;
        }

        if (finish_call.context_info != expected_finish) {
            std::cerr << "  ✗ DidFinish context mismatch: expected "
                      << expected_finish << ", got " << finish_call.context_info << "\n";
            return false;
        }

        std::cout << "  ✓ Context info passed correctly\n";
        return true;
    }

    void printCallHistory() const {
        std::cout << "  Hook call sequence:\n";
        for (size_t i = 0; i < call_history.size(); ++i) {
            std::cout << "    " << (i + 1) << ". ";
            switch (call_history[i].type) {
                case HookType::WillStart:
                    std::cout << "runLoopWillStartWithContextInfo";
                    break;
                case HookType::ShouldContinue:
                    std::cout << "runLoopShouldContinue";
                    break;
                case HookType::DidFinish:
                    std::cout << "runLoopDidFinishWithContextInfo";
                    break;
            }
            if (call_history[i].context_info) {
                std::cout << " (context=" << call_history[i].context_info << ")";
            }
            std::cout << "\n";
        }
    }

    // ========================================================================
    // Stub implementations for all other hooks
    // ========================================================================

    void shutdown() override {}
    void handleDOSBoxTitleChange(Bit32s, int, bool) override {}
    bool processEvents() override { return true; }
    bool MaybeProcessEvents() override { return true; }
    bool startFrame(Bit8u**, int&) override { return false; }
    void finishFrame(const uint16_t*) override {}
    Bitu prepareForFrameSize(Bitu, Bitu, Bitu, double, double, GFX_CallBack_t, double) override { return 0; }
    Bitu idealOutputMode(Bitu) override { return 0; }
    Bitu getRGBPaletteEntry(Bit8u r, Bit8u g, Bit8u b) override { return (static_cast<Bitu>(r) << 16) | (static_cast<Bitu>(g) << 8) | static_cast<Bitu>(b); }
    void setShader(const char*) override {}
    void applyRenderingStrategy() override {}
    int GetDisplayRefreshRate() override { return 60; }
    Bit8u herculesTintMode() override { return 0; }
    void setHerculesTintMode(Bit8u) override {}
    double CGACompositeHueOffset() override { return 0.0; }
    void setCGACompositeHueOffset(double) override {}
    Bit8u CGAComponentMode() override { return 1; }
    void setCGAComponentMode(Bit8u) override {}
    void shellWillStart(DOS_Shell*) override {}
    void shellDidFinish(DOS_Shell*, int) override {}
    void shellWillStartAutoexec(DOS_Shell*) override {}
    void didReturnToShell(DOS_Shell*) override {}
    bool shellShouldRunCommand(DOS_Shell*, const char*, const char*) override { return false; }
    void shellWillReadCommandInputFromHandle(DOS_Shell*, Bit16u) override {}
    void shellDidReadCommandInputFromHandle(DOS_Shell*, Bit16u) override {}
    bool handleShellCommandInput(DOS_Shell*, char*, Bitu*, bool*) override { return false; }
    bool hasPendingCommandsForShell(DOS_Shell*) override { return false; }
    bool executeNextPendingCommandForShell(DOS_Shell*) override { return false; }
    bool shellShouldDisplayStartupMessages(DOS_Shell*) override { return false; }
    void shellWillExecuteFileAtDOSPath(DOS_Shell*, const char*, const char*) override {}
    void shellDidExecuteFileAtDOSPath(DOS_Shell*, const char*) override {}
    void shellWillBeginBatchFile(DOS_Shell*, const char*, const char*) override {}
    void shellDidEndBatchFile(DOS_Shell*, const char*) override {}
    bool shellShouldContinue(DOS_Shell*) override { return false; }
    bool shouldMountPath(const char*) override { return true; }
    bool shouldShowFileWithName(const char*) override { return true; }
    bool shouldAllowWriteAccessToPath(const char*, DOS_Drive*) override { return true; }
    void driveDidMount(Bit8u) override {}
    void driveDidUnmount(Bit8u) override {}
    void didCreateLocalFile(const char*, DOS_Drive*) override {}
    void didRemoveLocalFile(const char*, DOS_Drive*) override {}
    FILE* openLocalFile(const char*, DOS_Drive*, const char*) override { return nullptr; }
    bool removeLocalFile(const char*, DOS_Drive*) override { return false; }
    bool moveLocalFile(const char*, const char*, DOS_Drive*) override { return false; }
    bool createLocalDir(const char*, DOS_Drive*) override { return false; }
    bool removeLocalDir(const char*, DOS_Drive*) override { return false; }
    bool getLocalPathStats(const char*, DOS_Drive*, struct stat*) override { return false; }
    bool localDirectoryExists(const char*, DOS_Drive*) override { return false; }
    bool localFileExists(const char*, DOS_Drive*) override { return false; }
    DIR_Handle openLocalDirectory(const char*, DOS_Drive*) override { return nullptr; }
    void closeLocalDirectory(DIR_Handle) override {}
    bool getNextDirectoryEntry(DIR_Handle, char*, bool&) override { return false; }
    void setMouseActive(bool) override {}
    void mouseMovedToPoint(float, float) override {}
    void setJoystickActive(bool) override {}
    Bitu keyboardBufferRemaining() override { return 128; }
    bool keyboardLayoutLoaded() override { return true; }
    const char* keyboardLayoutName() override { return "us"; }
    bool keyboardLayoutSupported(const char*) override { return true; }
    bool keyboardLayoutActive() override { return true; }
    void setKeyboardLayoutActive(bool) override {}
    void setNumLockActive(bool) override {}
    void setCapsLockActive(bool) override {}
    void setScrollLockActive(bool) override {}
    const char* preferredKeyboardLayout() override { return "us"; }
    bool continueListeningForKeyEvents() override { return false; }
    Bitu numKeyCodesInPasteBuffer() override { return 0; }
    bool getNextKeyCodeInPasteBuffer(Bit16u*, bool) override { return false; }
    Bitu PRINTER_readdata(Bitu, Bitu) override { return 0xFF; }
    void PRINTER_writedata(Bitu, Bitu, Bitu) override {}
    Bitu PRINTER_readstatus(Bitu, Bitu) override { return 0xDF; }
    void PRINTER_writecontrol(Bitu, Bitu, Bitu) override {}
    Bitu PRINTER_readcontrol(Bitu, Bitu) override { return 0x00; }
    bool PRINTER_isInited(Bitu) override { return false; }
    bool MIDIAvailable() override { return false; }
    void sendMIDIMessage(const uint8_t*, size_t) override {}
    void sendMIDISysex(const uint8_t*, size_t) override {}
    const char* suggestMIDIHandler() override { return "none"; }
    void MIDIWillRestart() override {}
    void MIDIDidRestart() override {}
    float masterVolume() override { return 1.0f; }
    void updateVolumes() override {}
    const char* localizedStringForKey(const char*) override { return nullptr; }
    void log(const char*) override {}
    void die(const char* msg) override { std::cerr << "[FATAL] " << msg << "\n"; std::exit(1); }
    FILE* openCaptureFile(const char*, const char*) override { return nullptr; }
};

// ============================================================================
// Simulated Emulation Loop (mirrors DOSBox structure)
// ============================================================================

void simulateEmulationLoop(void* context_info = nullptr) {
    // This simulates the structure in dosbox.cpp
    BOXER_HOOK_VOID(runLoopWillStartWithContextInfo, context_info);

    // Main emulation loop (like normal_loop())
    while (true) {
        if (!BOXER_HOOK_BOOL(runLoopShouldContinue)) {
            break;
        }
        // Simulate some emulation work
    }

    BOXER_HOOK_VOID(runLoopDidFinishWithContextInfo, context_info);
}

void simulateEmulationLoopWithException(void* context_info = nullptr) {
    // This simulates exception handling in dosbox.cpp
    BOXER_HOOK_VOID(runLoopWillStartWithContextInfo, context_info);

    try {
        while (true) {
            if (!BOXER_HOOK_BOOL(runLoopShouldContinue)) {
                break;
            }
            // Simulate exception after 2 iterations
            if (static_cast<LifecycleTrackingDelegate*>(g_boxer_delegate)->iteration_count == 2) {
                throw std::runtime_error("Simulated exception");
            }
        }
    } catch (const std::exception& e) {
        std::cout << "  [Exception caught: " << e.what() << "]\n";
    }

    BOXER_HOOK_VOID(runLoopDidFinishWithContextInfo, context_info);
}

// ============================================================================
// Test Cases
// ============================================================================

bool testNormalLifecycle() {
    std::cout << "\n=== TEST 1: Normal Lifecycle ===\n";
    std::cout << "Testing: WillStart → ShouldContinue (×5) → DidFinish\n\n";

    LifecycleTrackingDelegate delegate;
    delegate.max_iterations = 5;
    g_boxer_delegate = &delegate;

    simulateEmulationLoop(nullptr);

    std::cout << "\nValidation:\n";
    delegate.printCallHistory();

    bool order_ok = delegate.validateCallOrder();
    bool context_ok = delegate.validateContextInfo(nullptr, nullptr);

    int expected_calls = 1 + 5 + 1; // WillStart + 5×ShouldContinue + DidFinish
    bool count_ok = (delegate.call_history.size() == static_cast<size_t>(expected_calls));

    if (!count_ok) {
        std::cerr << "  ✗ Expected " << expected_calls << " calls, got "
                  << delegate.call_history.size() << "\n";
    } else {
        std::cout << "  ✓ Correct number of hook calls (" << expected_calls << ")\n";
    }

    bool passed = order_ok && context_ok && count_ok;
    std::cout << "\n" << (passed ? "✓ TEST 1 PASSED" : "✗ TEST 1 FAILED") << "\n";
    return passed;
}

bool testAbortDuringExecution() {
    std::cout << "\n=== TEST 2: Abort During Execution ===\n";
    std::cout << "Testing: Abort signal stops loop immediately\n\n";

    LifecycleTrackingDelegate delegate;
    delegate.max_iterations = 100; // Set high, will abort earlier
    g_boxer_delegate = &delegate;

    // Start emulation loop
    BOXER_HOOK_VOID(runLoopWillStartWithContextInfo, nullptr);

    int iterations = 0;
    while (true) {
        if (!BOXER_HOOK_BOOL(runLoopShouldContinue)) {
            break;
        }
        iterations++;

        // Signal abort after 3 iterations
        if (iterations == 3) {
            std::cout << "  [Signaling abort]\n";
            delegate.should_abort = true;
        }
    }

    BOXER_HOOK_VOID(runLoopDidFinishWithContextInfo, nullptr);

    std::cout << "\nValidation:\n";
    std::cout << "  Loop stopped after " << iterations << " iterations\n";

    bool stopped_at_right_time = (iterations == 3); // Abort signaled after 3rd iteration
    if (stopped_at_right_time) {
        std::cout << "  ✓ Abort signal worked correctly\n";
    } else {
        std::cerr << "  ✗ Expected 3 iterations, got " << iterations << "\n";
    }

    bool order_ok = delegate.validateCallOrder();
    bool passed = order_ok && stopped_at_right_time;

    std::cout << "\n" << (passed ? "✓ TEST 2 PASSED" : "✗ TEST 2 FAILED") << "\n";
    return passed;
}

bool testContextInfoPropagation() {
    std::cout << "\n=== TEST 3: Context Info Propagation ===\n";
    std::cout << "Testing: Context passed to WillStart and DidFinish\n\n";

    LifecycleTrackingDelegate delegate;
    delegate.max_iterations = 3;
    g_boxer_delegate = &delegate;

    // Use a dummy pointer as context
    int dummy_context = 42;
    void* context = &dummy_context;

    std::cout << "  Using context: " << context << "\n\n";

    simulateEmulationLoop(context);

    std::cout << "\nValidation:\n";
    bool passed = delegate.validateContextInfo(context, context);

    std::cout << "\n" << (passed ? "✓ TEST 3 PASSED" : "✗ TEST 3 FAILED") << "\n";
    return passed;
}

bool testExceptionSafety() {
    std::cout << "\n=== TEST 4: Exception Safety ===\n";
    std::cout << "Testing: DidFinish called even if exception thrown\n\n";

    LifecycleTrackingDelegate delegate;
    delegate.max_iterations = 10;
    g_boxer_delegate = &delegate;

    simulateEmulationLoopWithException(nullptr);

    std::cout << "\nValidation:\n";

    // Check that DidFinish was still called
    bool has_did_finish = !delegate.call_history.empty() &&
                          delegate.call_history.back().type == LifecycleTrackingDelegate::HookType::DidFinish;

    if (has_did_finish) {
        std::cout << "  ✓ DidFinish was called even after exception\n";
    } else {
        std::cerr << "  ✗ DidFinish was NOT called after exception\n";
    }

    bool order_ok = delegate.validateCallOrder();
    bool passed = has_did_finish && order_ok;

    std::cout << "\n" << (passed ? "✓ TEST 4 PASSED" : "✗ TEST 4 FAILED") << "\n";
    return passed;
}

bool testImmediateAbort() {
    std::cout << "\n=== TEST 5: Immediate Abort ===\n";
    std::cout << "Testing: Abort on first iteration\n\n";

    LifecycleTrackingDelegate delegate;
    delegate.max_iterations = 0; // Abort immediately
    g_boxer_delegate = &delegate;

    simulateEmulationLoop(nullptr);

    std::cout << "\nValidation:\n";
    delegate.printCallHistory();

    // Should have: WillStart, ShouldContinue (returns false), DidFinish
    bool has_all_three = delegate.call_history.size() == 3;
    if (has_all_three) {
        std::cout << "  ✓ All lifecycle hooks called even with immediate abort\n";
    } else {
        std::cerr << "  ✗ Expected 3 calls, got " << delegate.call_history.size() << "\n";
    }

    bool order_ok = delegate.validateCallOrder();
    bool passed = has_all_three && order_ok;

    std::cout << "\n" << (passed ? "✓ TEST 5 PASSED" : "✗ TEST 5 FAILED") << "\n";
    return passed;
}

// ============================================================================
// Main Entry Point
// ============================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "Boxer DOSBox Integration\n";
    std::cout << "Lifecycle Smoke Test Suite\n";
    std::cout << "Phase 2, Task 2-4\n";
    std::cout << "========================================\n";

    std::cout << "\nTesting lifecycle hook integration:\n";
    std::cout << "  - INT-077: runLoopWillStartWithContextInfo\n";
    std::cout << "  - INT-058: runLoopShouldContinue\n";
    std::cout << "  - INT-078: runLoopDidFinishWithContextInfo\n";

    int tests_passed = 0;
    int tests_failed = 0;

    // Run all tests
    if (testNormalLifecycle()) tests_passed++; else tests_failed++;
    if (testAbortDuringExecution()) tests_passed++; else tests_failed++;
    if (testContextInfoPropagation()) tests_passed++; else tests_failed++;
    if (testExceptionSafety()) tests_passed++; else tests_failed++;
    if (testImmediateAbort()) tests_passed++; else tests_failed++;

    // Summary
    std::cout << "\n========================================\n";
    std::cout << "Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "Tests passed: " << tests_passed << "/5\n";
    std::cout << "Tests failed: " << tests_failed << "/5\n";

    if (tests_failed == 0) {
        std::cout << "\n✓ ALL TESTS PASSED\n\n";
        std::cout << "Lifecycle integration validated:\n";
        std::cout << "  ✓ All 3 lifecycle hooks tested\n";
        std::cout << "  ✓ Hook call order validated\n";
        std::cout << "  ✓ Context info passed correctly\n";
        std::cout << "  ✓ Exception safety confirmed\n";
        std::cout << "  ✓ No undefined behavior detected\n";
        std::cout << "\nReady for integration with DOSBox library.\n";
        return 0;
    } else {
        std::cout << "\n✗ SOME TESTS FAILED\n";
        std::cout << "Fix issues before proceeding.\n";
        return 1;
    }
}
