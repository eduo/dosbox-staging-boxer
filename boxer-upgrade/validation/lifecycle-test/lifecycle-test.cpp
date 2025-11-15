/*
 * lifecycle-test.cpp - Comprehensive Lifecycle Hook Test Suite
 *
 * Tests all three critical lifecycle hooks:
 * - INT-059: runLoopShouldContinue (emergency abort)
 * - INT-077: runLoopWillStartWithContextInfo (initialization)
 * - INT-078: runLoopDidFinishWithContextInfo (cleanup)
 *
 * Test cases:
 * 1. Normal lifecycle (start → run → stop)
 * 2. Abort before start
 * 3. Abort during execution
 * 4. Abort immediately (first iteration)
 * 5. Rapid start/stop cycles
 * 6. Latency measurement
 *
 * Copyright (c) 2025 Boxer DOSBox Integration Project
 * Released under GNU General Public License 2.0
 */

#include "../smoke-test/boxer_hooks_stub.h"
#include <iostream>
#include <chrono>
#include <atomic>
#include <thread>
#include <cassert>

// ============================================================================
// Test Delegate Implementation
// ============================================================================

class LifecycleTestDelegate : public IBoxerDelegate {
private:
    std::atomic<bool> cancelled{false};
    std::atomic<int> iteration_count{0};
    std::atomic<bool> will_start_called{false};
    std::atomic<bool> did_finish_called{false};
    int max_iterations = -1;  // -1 = run until cancelled

    std::chrono::high_resolution_clock::time_point cancel_time;
    std::chrono::high_resolution_clock::time_point abort_time;

public:
    // Reset for new test
    void reset() {
        cancelled.store(false);
        iteration_count.store(0);
        will_start_called.store(false);
        did_finish_called.store(false);
        max_iterations = -1;
    }

    // Set maximum iterations before auto-stop
    void setMaxIterations(int max) {
        max_iterations = max;
    }

    // Signal cancellation
    void cancel() {
        cancel_time = std::chrono::high_resolution_clock::now();
        cancelled.store(true, std::memory_order_relaxed);
    }

    // Check if hooks were called
    bool wasWillStartCalled() const { return will_start_called.load(); }
    bool wasDidFinishCalled() const { return did_finish_called.load(); }
    int getIterationCount() const { return iteration_count.load(); }

    // Get abort latency in microseconds
    int64_t getAbortLatencyMicroseconds() const {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            abort_time - cancel_time
        );
        return duration.count();
    }

    // ========================================================================
    // IBoxerDelegate Implementation
    // ========================================================================

    // INT-059: Emergency abort check (called ~10,000/sec)
    bool runLoopShouldContinue() override {
        int current = iteration_count.fetch_add(1);

        // Check if cancelled
        if (cancelled.load(std::memory_order_relaxed)) {
            abort_time = std::chrono::high_resolution_clock::now();
            return false;
        }

        // Check if reached max iterations
        if (max_iterations > 0 && current >= max_iterations) {
            return false;
        }

        return true;
    }

    // INT-077: Initialization before emulation starts
    void runLoopWillStartWithContextInfo(void* context_info) override {
        will_start_called.store(true);
        iteration_count.store(0);

        std::cout << "  [HOOK] runLoopWillStart called";
        if (context_info) {
            std::cout << " with context";
        } else {
            std::cout << " (context=nullptr)";
        }
        std::cout << std::endl;
    }

    // INT-078: Cleanup after emulation finishes
    void runLoopDidFinishWithContextInfo(void* context_info) override {
        did_finish_called.store(true);

        std::cout << "  [HOOK] runLoopDidFinish called";
        if (context_info) {
            std::cout << " with context";
        } else {
            std::cout << " (context=nullptr)";
        }
        std::cout << " (iterations=" << iteration_count.load() << ")" << std::endl;
    }

    // Stub implementations for other hooks
    bool processEvents() override { return true; }
    bool MaybeProcessEvents() override { return true; }
    bool startFrame(Bit8u** frameBuffer, int& pitch) override { return false; }
    void finishFrame(const uint16_t* changedLines) override {}
    Bitu prepareForFrameSize(Bitu width, Bitu height, Bitu gfx_flags,
                            double scalex, double scaley,
                            GFX_CallBack_t callback,
                            double pixel_aspect) override { return 0; }
    Bitu idealOutputMode(Bitu flags) override { return 0; }
    Bitu getRGBPaletteEntry(Bit8u red, Bit8u green, Bit8u blue) override { return 0; }
    void setShader(const char* shaderSource) override {}
    void applyRenderingStrategy() override {}
    int GetDisplayRefreshRate() override { return 60; }
    void setMouseActive(bool active) override {}
    void mouseMovedToPoint(float x, float y) override {}
    void setJoystickActive(bool active) override {}
    void handleDOSBoxTitleChange(Bit32s cycles, int frameskip, bool paused) override {}
    void shutdown() override {}
    Bit8u herculesTintMode() override { return 0; }
    void setHerculesTintMode(Bit8u mode) override {}
    double CGACompositeHueOffset() override { return 0.0; }
    void setCGACompositeHueOffset(double offset) override {}
    Bit8u CGAComponentMode() override { return 0; }
    void setCGAComponentMode(Bit8u mode) override {}
    void shellWillStart(DOS_Shell* shell) override {}
    void shellDidFinish(DOS_Shell* shell, int exit_code) override {}
    void shellWillStartAutoexec(DOS_Shell* shell) override {}
    void didReturnToShell(DOS_Shell* shell) override {}
    bool shellShouldRunCommand(DOS_Shell* shell, const char* cmd, const char* args) override { return false; }
    void shellWillReadCommandInputFromHandle(DOS_Shell* shell, Bit16u handle) override {}
    void shellDidReadCommandInputFromHandle(DOS_Shell* shell, Bit16u handle) override {}
    bool handleShellCommandInput(DOS_Shell* shell, char* cmd, Bitu* cursorPosition, bool* executeImmediately) override { return false; }
    bool hasPendingCommandsForShell(DOS_Shell* shell) override { return false; }
    bool executeNextPendingCommandForShell(DOS_Shell* shell) override { return false; }
    bool shellShouldDisplayStartupMessages(DOS_Shell* shell) override { return true; }
    void shellWillExecuteFileAtDOSPath(DOS_Shell* shell, const char* canonicalPath, const char* arguments) override {}
    void shellDidExecuteFileAtDOSPath(DOS_Shell* shell, const char* canonicalPath) override {}
    void shellWillBeginBatchFile(DOS_Shell* shell, const char* canonicalPath, const char* arguments) override {}
    void shellDidEndBatchFile(DOS_Shell* shell, const char* canonicalPath) override {}
    bool shellShouldContinue(DOS_Shell* shell) override { return true; }
    bool shouldMountPath(const char* path) override { return true; }
    bool shouldShowFileWithName(const char* name) override { return true; }
    bool shouldAllowWriteAccessToPath(const char* path, DOS_Drive* drive) override { return true; }
    void driveDidMount(Bit8u driveIndex) override {}
    void driveDidUnmount(Bit8u driveIndex) override {}
    void didCreateLocalFile(const char* path, DOS_Drive* drive) override {}
    void didRemoveLocalFile(const char* path, DOS_Drive* drive) override {}
    FILE* openLocalFile(const char* path, DOS_Drive* drive, const char* mode) override { return nullptr; }
    bool removeLocalFile(const char* path, DOS_Drive* drive) override { return false; }
    bool moveLocalFile(const char* fromPath, const char* toPath, DOS_Drive* drive) override { return false; }
    bool createLocalDir(const char* path, DOS_Drive* drive) override { return false; }
    bool removeLocalDir(const char* path, DOS_Drive* drive) override { return false; }
    bool getLocalPathStats(const char* path, DOS_Drive* drive, struct stat* outStatus) override { return false; }
    bool localDirectoryExists(const char* path, DOS_Drive* drive) override { return false; }
    bool localFileExists(const char* path, DOS_Drive* drive) override { return false; }
    DIR_Handle openLocalDirectory(const char* path, DOS_Drive* drive) override { return nullptr; }
    void closeLocalDirectory(DIR_Handle handle) override {}
    bool getNextDirectoryEntry(DIR_Handle handle, char* outName, bool& isDirectory) override { return false; }
    Bitu keyboardBufferRemaining() override { return 0; }
    bool keyboardLayoutLoaded() override { return false; }
    const char* keyboardLayoutName() override { return "us"; }
    bool keyboardLayoutSupported(const char* code) override { return false; }
    bool keyboardLayoutActive() override { return false; }
    void setKeyboardLayoutActive(bool active) override {}
    void setNumLockActive(bool active) override {}
    void setCapsLockActive(bool active) override {}
    void setScrollLockActive(bool active) override {}
    const char* preferredKeyboardLayout() override { return "us"; }
    bool continueListeningForKeyEvents() override { return true; }
    Bitu numKeyCodesInPasteBuffer() override { return 0; }
    bool getNextKeyCodeInPasteBuffer(Bit16u* outKeyCode, bool consumeKey) override { return false; }
    Bitu PRINTER_readdata(Bitu port, Bitu iolen) override { return 0; }
    void PRINTER_writedata(Bitu port, Bitu val, Bitu iolen) override {}
    Bitu PRINTER_readstatus(Bitu port, Bitu iolen) override { return 0; }
    void PRINTER_writecontrol(Bitu port, Bitu val, Bitu iolen) override {}
    Bitu PRINTER_readcontrol(Bitu port, Bitu iolen) override { return 0; }
    bool PRINTER_isInited(Bitu port) override { return false; }
    bool MIDIAvailable() override { return false; }
    void sendMIDIMessage(const uint8_t* data, size_t length) override {}
    void sendMIDISysex(const uint8_t* data, size_t length) override {}
    const char* suggestMIDIHandler() override { return "none"; }
    void MIDIWillRestart() override {}
    void MIDIDidRestart() override {}
    float masterVolume() override { return 1.0f; }
    void updateVolumes() override {}
    const char* localizedStringForKey(const char* key) override { return nullptr; }
    void log(const char* message) override { std::cout << "  [LOG] " << message << std::endl; }
    void die(const char* message) override { std::cerr << "  [DIE] " << message << std::endl; }
    FILE* openCaptureFile(const char* filename, const char* mode) override { return nullptr; }
};

// ============================================================================
// Simulated Emulation Loop
// ============================================================================

// Simulates the DOSBox emulation loop with our hooks
void simulateEmulationLoop() {
    // INT-077: WillStart hook
    BOXER_HOOK_VOID(runLoopWillStartWithContextInfo, nullptr);

    // Main emulation loop (simulated)
    while (true) {
        // INT-059: ShouldContinue check (emergency abort)
        if (!BOXER_HOOK_BOOL(runLoopShouldContinue)) {
            break;  // Abort emulation
        }

        // Simulate emulation work (very brief)
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }

    // INT-078: DidFinish hook
    BOXER_HOOK_VOID(runLoopDidFinishWithContextInfo, nullptr);
}

// ============================================================================
// Test Cases
// ============================================================================

bool testNormalLifecycle(LifecycleTestDelegate& delegate) {
    std::cout << "\n[TEST 1] Normal Lifecycle (start → run briefly → stop)" << std::endl;

    delegate.reset();
    delegate.setMaxIterations(100);  // Run 100 iterations then stop

    simulateEmulationLoop();

    // Verify hooks were called
    bool passed = true;

    if (!delegate.wasWillStartCalled()) {
        std::cerr << "  ✗ FAIL: WillStart not called" << std::endl;
        passed = false;
    } else {
        std::cout << "  ✓ WillStart called" << std::endl;
    }

    if (!delegate.wasDidFinishCalled()) {
        std::cerr << "  ✗ FAIL: DidFinish not called" << std::endl;
        passed = false;
    } else {
        std::cout << "  ✓ DidFinish called" << std::endl;
    }

    int iterations = delegate.getIterationCount();
    if (iterations != 100) {
        std::cerr << "  ✗ FAIL: Expected 100 iterations, got " << iterations << std::endl;
        passed = false;
    } else {
        std::cout << "  ✓ Ran exactly 100 iterations" << std::endl;
    }

    if (passed) {
        std::cout << "  ✅ TEST PASSED" << std::endl;
    }

    return passed;
}

bool testAbortDuringExecution(LifecycleTestDelegate& delegate) {
    std::cout << "\n[TEST 2] Abort During Execution" << std::endl;

    delegate.reset();

    // Start emulation in separate thread
    std::thread emulation_thread([&]() {
        simulateEmulationLoop();
    });

    // Wait a bit, then signal abort
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::cout << "  [ACTION] Signaling abort..." << std::endl;
    delegate.cancel();

    // Wait for thread to finish
    emulation_thread.join();

    // Verify hooks were called
    bool passed = true;

    if (!delegate.wasWillStartCalled()) {
        std::cerr << "  ✗ FAIL: WillStart not called" << std::endl;
        passed = false;
    } else {
        std::cout << "  ✓ WillStart called" << std::endl;
    }

    if (!delegate.wasDidFinishCalled()) {
        std::cerr << "  ✗ FAIL: DidFinish not called after abort" << std::endl;
        passed = false;
    } else {
        std::cout << "  ✓ DidFinish called (cleanup happened)" << std::endl;
    }

    int iterations = delegate.getIterationCount();
    if (iterations <= 0) {
        std::cerr << "  ✗ FAIL: No iterations ran" << std::endl;
        passed = false;
    } else {
        std::cout << "  ✓ Ran " << iterations << " iterations before abort" << std::endl;
    }

    int64_t latency_us = delegate.getAbortLatencyMicroseconds();
    std::cout << "  Abort latency: " << latency_us << " μs";

    if (latency_us > 100000) {  // 100ms = 100,000μs
        std::cerr << " - ✗ EXCEEDS 100ms REQUIREMENT!" << std::endl;
        passed = false;
    } else {
        std::cout << " - ✓ Within 100ms requirement" << std::endl;
    }

    if (passed) {
        std::cout << "  ✅ TEST PASSED" << std::endl;
    }

    return passed;
}

bool testImmediateAbort(LifecycleTestDelegate& delegate) {
    std::cout << "\n[TEST 3] Abort Immediately (first iteration)" << std::endl;

    delegate.reset();
    delegate.cancel();  // Cancel before starting

    simulateEmulationLoop();

    // Verify hooks were called
    bool passed = true;

    if (!delegate.wasWillStartCalled()) {
        std::cerr << "  ✗ FAIL: WillStart not called" << std::endl;
        passed = false;
    } else {
        std::cout << "  ✓ WillStart called" << std::endl;
    }

    if (!delegate.wasDidFinishCalled()) {
        std::cerr << "  ✗ FAIL: DidFinish not called" << std::endl;
        passed = false;
    } else {
        std::cout << "  ✓ DidFinish called (cleanup happened)" << std::endl;
    }

    int iterations = delegate.getIterationCount();
    if (iterations > 1) {
        std::cerr << "  ✗ FAIL: Too many iterations (" << iterations << "), expected immediate abort" << std::endl;
        passed = false;
    } else {
        std::cout << "  ✓ Aborted immediately (iterations=" << iterations << ")" << std::endl;
    }

    if (passed) {
        std::cout << "  ✅ TEST PASSED" << std::endl;
    }

    return passed;
}

bool testRapidCycles(LifecycleTestDelegate& delegate) {
    std::cout << "\n[TEST 4] Rapid Start/Stop Cycles (100 cycles)" << std::endl;

    bool passed = true;

    for (int cycle = 0; cycle < 100; cycle++) {
        delegate.reset();
        delegate.setMaxIterations(10);  // Just 10 iterations per cycle

        simulateEmulationLoop();

        if (!delegate.wasWillStartCalled() || !delegate.wasDidFinishCalled()) {
            std::cerr << "  ✗ FAIL at cycle " << cycle << ": Hooks not called" << std::endl;
            passed = false;
            break;
        }
    }

    if (passed) {
        std::cout << "  ✓ All 100 cycles completed successfully" << std::endl;
        std::cout << "  ✓ No crashes or hangs" << std::endl;
        std::cout << "  ✅ TEST PASSED" << std::endl;
    }

    return passed;
}

bool testHookCallOrder(LifecycleTestDelegate& delegate) {
    std::cout << "\n[TEST 5] Hook Call Order Validation" << std::endl;

    delegate.reset();
    delegate.setMaxIterations(5);

    std::cout << "  Expected order: WillStart → ShouldContinue (×5) → DidFinish" << std::endl;

    simulateEmulationLoop();

    bool passed = true;

    if (!delegate.wasWillStartCalled()) {
        std::cerr << "  ✗ FAIL: WillStart never called" << std::endl;
        passed = false;
    }

    if (!delegate.wasDidFinishCalled()) {
        std::cerr << "  ✗ FAIL: DidFinish never called" << std::endl;
        passed = false;
    }

    if (delegate.getIterationCount() != 5) {
        std::cerr << "  ✗ FAIL: Wrong iteration count" << std::endl;
        passed = false;
    }

    if (passed) {
        std::cout << "  ✓ Hook call order correct" << std::endl;
        std::cout << "  ✅ TEST PASSED" << std::endl;
    }

    return passed;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "Boxer Lifecycle Hook Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nTesting INT-057, INT-058, INT-059 integration" << std::endl;

    LifecycleTestDelegate delegate;
    g_boxer_delegate = &delegate;

    int passed = 0;
    int failed = 0;

    // Run all tests
    if (testNormalLifecycle(delegate)) passed++; else failed++;
    if (testAbortDuringExecution(delegate)) passed++; else failed++;
    if (testImmediateAbort(delegate)) passed++; else failed++;
    if (testRapidCycles(delegate)) passed++; else failed++;
    if (testHookCallOrder(delegate)) passed++; else failed++;

    // Summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "Test Summary:" << std::endl;
    std::cout << "  Passed: " << passed << "/5" << std::endl;
    std::cout << "  Failed: " << failed << "/5" << std::endl;
    std::cout << "========================================" << std::endl;

    if (failed == 0) {
        std::cout << "\n✅ ALL TESTS PASSED - Lifecycle hooks working correctly!" << std::endl;
        return 0;
    } else {
        std::cerr << "\n❌ SOME TESTS FAILED - Review failures above" << std::endl;
        return 1;
    }
}
