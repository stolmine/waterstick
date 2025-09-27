#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <random>
#include <iomanip>
#include <fstream>
#include <memory>
#include <cmath>
#include <atomic>

// Forward declarations to avoid VST3 SDK dependencies
namespace WaterStick {

// Minimal pitch shifting delay line class for standalone testing
class TestPitchDelayLine {
public:
    TestPitchDelayLine() :
        mBufferSize(0),
        mWriteIndex(0),
        mReadPosition(0.0f),
        mPitchSemitones(0),
        mPitchRatio(1.0f),
        mTargetPitchRatio(1.0f),
        mSmoothingCoeff(0.001f),
        mSampleRate(44100.0),
        mEmergencyBypassMode(false),
        mProcessingTimeouts(0),
        mInfiniteLoopPrevention(0)
    {}

    ~TestPitchDelayLine() = default;

    void initialize(double sampleRate, double maxDelaySeconds) {
        mSampleRate = sampleRate;
        mBufferSize = static_cast<int>(sampleRate * maxDelaySeconds * 4); // 4x safety buffer
        mBuffer.resize(mBufferSize, 0.0f);
        mWriteIndex = 0;
        mReadPosition = 0.0f;
        mEmergencyBypassMode = false;
        mProcessingTimeouts = 0;
        mInfiniteLoopPrevention = 0;
    }

    void setPitchShift(int semitones) {
        if (semitones != mPitchSemitones) {
            mPitchSemitones = semitones;
            mTargetPitchRatio = std::pow(2.0f, semitones / 12.0f);

            // Log pitch changes that might cause dropouts
            if (std::abs(semitones) > 6) {
                logMessage("Extreme pitch shift: " + std::to_string(semitones) + " semitones");
            }
        }
    }

    float processSample(float input) {
        auto startTime = std::chrono::high_resolution_clock::now();

        // Emergency bypass check
        if (mEmergencyBypassMode) {
            return input * 0.1f; // Attenuated bypass
        }

        // Write input to buffer
        mBuffer[mWriteIndex] = input;
        mWriteIndex = (mWriteIndex + 1) % mBufferSize;

        // Update pitch ratio with smoothing
        updatePitchRatioSmoothing();

        // Read with variable speed for pitch shifting
        float output = 0.0f;
        if (mPitchRatio != 1.0f) {
            output = processVariableSpeedRead();
        } else {
            // Direct delay line read for 0 semitones
            int readIndex = (mWriteIndex - 2205) % mBufferSize; // ~50ms delay
            if (readIndex < 0) readIndex += mBufferSize;
            output = mBuffer[readIndex];
        }

        // Check for processing timeout
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

        if (duration.count() > 100) { // 100 microseconds threshold
            mProcessingTimeouts++;
            if (mProcessingTimeouts > 1000) {
                enterEmergencyBypass("Processing timeout detected");
            }
        }

        return output;
    }

    // Performance monitoring methods
    int getTimeoutCount() const { return mProcessingTimeouts; }
    int getLoopPreventionCount() const { return mInfiniteLoopPrevention; }
    bool isInEmergencyBypass() const { return mEmergencyBypassMode; }

    void resetPerformanceCounters() {
        mProcessingTimeouts = 0;
        mInfiniteLoopPrevention = 0;
        mEmergencyBypassMode = false;
    }

private:
    std::vector<float> mBuffer;
    int mBufferSize;
    int mWriteIndex;
    float mReadPosition;
    double mSampleRate;

    // Pitch shifting parameters
    int mPitchSemitones;
    float mPitchRatio;
    float mTargetPitchRatio;
    float mSmoothingCoeff;

    // Performance monitoring
    std::atomic<bool> mEmergencyBypassMode;
    std::atomic<int> mProcessingTimeouts;
    std::atomic<int> mInfiniteLoopPrevention;

    void updatePitchRatioSmoothing() {
        if (std::abs(mPitchRatio - mTargetPitchRatio) > 0.0001f) {
            mPitchRatio += (mTargetPitchRatio - mPitchRatio) * mSmoothingCoeff;
        }
    }

    float processVariableSpeedRead() {
        // Advance read position based on pitch ratio
        mReadPosition += mPitchRatio;

        // Prevent infinite loops in extreme cases
        int loopCounter = 0;
        while (mReadPosition >= mBufferSize && loopCounter < 1000) {
            mReadPosition -= mBufferSize;
            loopCounter++;
        }

        if (loopCounter >= 1000) {
            mInfiniteLoopPrevention++;
            enterEmergencyBypass("Infinite loop in variable speed read");
            return 0.0f;
        }

        // Interpolated read
        int intPos = static_cast<int>(mReadPosition);
        float fracPos = mReadPosition - intPos;

        if (intPos >= 0 && intPos < mBufferSize - 1) {
            return mBuffer[intPos] * (1.0f - fracPos) + mBuffer[intPos + 1] * fracPos;
        }

        return 0.0f;
    }

    void enterEmergencyBypass(const std::string& reason) {
        mEmergencyBypassMode = true;
        logMessage("Emergency bypass activated: " + reason);
    }

    void logMessage(const std::string& message) {
        std::cout << "[TestPitchDelayLine] " << message << std::endl;
    }
};

// Test orchestrator class
class PitchShiftDropoutTester {
public:
    PitchShiftDropoutTester() : mSampleRate(44100.0), mTestDuration(10.0) {
        // Initialize 16 delay lines for testing
        for (int i = 0; i < 16; ++i) {
            mDelayLines[i] = std::make_unique<TestPitchDelayLine>();
            mDelayLines[i]->initialize(mSampleRate, 2.0); // 2 second max delay
        }
    }

    struct TestScenario {
        std::string name;
        std::vector<int> pitchShifts;
        float parameterChangeRate; // Changes per second
        bool useExtremeValues;
        float testDuration;
    };

    struct PerformanceReport {
        std::string scenarioName;
        int totalTimeouts;
        int totalLoopPreventions;
        int emergencyBypassCount;
        double avgProcessingTimeUs;
        double maxProcessingTimeUs;
        bool dropoutDetected;
        std::vector<double> processingTimes;
    };

    std::vector<PerformanceReport> runAllTests() {
        std::vector<PerformanceReport> reports;

        auto scenarios = createTestScenarios();
        for (const auto& scenario : scenarios) {
            std::cout << "\n=== Running Test: " << scenario.name << " ===" << std::endl;
            auto report = runTestScenario(scenario);
            reports.push_back(report);

            // Brief pause between tests
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        return reports;
    }

private:
    static const int NUM_TAPS = 16;
    std::unique_ptr<TestPitchDelayLine> mDelayLines[NUM_TAPS];
    double mSampleRate;
    double mTestDuration;

    std::vector<TestScenario> createTestScenarios() {
        return {
            {
                "Extreme Pitch Changes",
                {-12, 12, -12, 12, 0, -6, 6, -12}, // Extreme values
                2.0f, // 2 changes per second
                true,
                5.0f
            },
            {
                "Rapid Parameter Updates",
                {1, -1, 2, -2, 3, -3, 4, -4}, // Moderate values, rapid changes
                10.0f, // 10 changes per second
                false,
                3.0f
            },
            {
                "Long Delay + Pitch Shift",
                {-8, -4, 0, 4, 8, -6, -2, 2}, // Various pitch values
                0.5f, // Slow changes
                true,
                8.0f
            },
            {
                "All Taps Simultaneous",
                {-12, -10, -8, -6, -4, -2, 0, 2, 4, 6, 8, 10, 12, -11, -9, -7}, // All 16 taps
                1.0f, // 1 change per second
                true,
                6.0f
            },
            {
                "Stress Test - Random",
                {}, // Will be filled randomly
                5.0f, // 5 changes per second
                true,
                10.0f
            }
        };
    }

    PerformanceReport runTestScenario(const TestScenario& scenario) {
        PerformanceReport report;
        report.scenarioName = scenario.name;
        report.totalTimeouts = 0;
        report.totalLoopPreventions = 0;
        report.emergencyBypassCount = 0;
        report.avgProcessingTimeUs = 0.0;
        report.maxProcessingTimeUs = 0.0;
        report.dropoutDetected = false;

        // Reset all delay lines
        for (int i = 0; i < NUM_TAPS; ++i) {
            mDelayLines[i]->resetPerformanceCounters();
        }

        // Generate test audio
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> audioDist(-1.0f, 1.0f);
        std::uniform_int_distribution<int> pitchDist(-12, 12);

        int totalSamples = static_cast<int>(scenario.testDuration * mSampleRate);
        int paramChangeInterval = static_cast<int>(mSampleRate / scenario.parameterChangeRate);

        auto pitchShifts = scenario.pitchShifts;
        if (scenario.name == "Stress Test - Random") {
            // Fill with random values
            pitchShifts.clear();
            for (int i = 0; i < NUM_TAPS; ++i) {
                pitchShifts.push_back(pitchDist(gen));
            }
        }

        std::cout << "Processing " << totalSamples << " samples..." << std::endl;

        for (int sample = 0; sample < totalSamples; ++sample) {
            // Update pitch parameters periodically
            if (sample % paramChangeInterval == 0 && !pitchShifts.empty()) {
                int numTapsToUpdate = std::min(NUM_TAPS, static_cast<int>(pitchShifts.size()));
                for (int tap = 0; tap < numTapsToUpdate; ++tap) {
                    int pitchIndex = tap % pitchShifts.size();
                    if (scenario.name == "Stress Test - Random") {
                        // Generate new random value
                        pitchShifts[pitchIndex] = pitchDist(gen);
                    }

                    auto start = std::chrono::high_resolution_clock::now();
                    mDelayLines[tap]->setPitchShift(pitchShifts[pitchIndex]);
                    auto end = std::chrono::high_resolution_clock::now();

                    double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 1000.0;
                    report.processingTimes.push_back(duration);
                }
            }

            // Generate test audio and process
            float inputL = audioDist(gen) * 0.5f;
            float inputR = audioDist(gen) * 0.5f;

            auto processingStart = std::chrono::high_resolution_clock::now();

            for (int tap = 0; tap < NUM_TAPS; ++tap) {
                float output = mDelayLines[tap]->processSample((tap % 2 == 0) ? inputL : inputR);
                // Accumulate output (would normally go to mixer)
                (void)output; // Suppress unused variable warning
            }

            auto processingEnd = std::chrono::high_resolution_clock::now();
            double processingTime = std::chrono::duration_cast<std::chrono::nanoseconds>(processingEnd - processingStart).count() / 1000.0;

            report.processingTimes.push_back(processingTime);

            if (processingTime > report.maxProcessingTimeUs) {
                report.maxProcessingTimeUs = processingTime;
            }

            // Progress indicator
            if (sample % (totalSamples / 10) == 0) {
                std::cout << "Progress: " << (sample * 100 / totalSamples) << "%" << std::endl;
            }
        }

        // Collect final statistics
        for (int i = 0; i < NUM_TAPS; ++i) {
            report.totalTimeouts += mDelayLines[i]->getTimeoutCount();
            report.totalLoopPreventions += mDelayLines[i]->getLoopPreventionCount();
            if (mDelayLines[i]->isInEmergencyBypass()) {
                report.emergencyBypassCount++;
            }
        }

        // Calculate averages
        if (!report.processingTimes.empty()) {
            double sum = 0.0;
            for (double time : report.processingTimes) {
                sum += time;
            }
            report.avgProcessingTimeUs = sum / report.processingTimes.size();
        }

        // Detect dropouts (processing time > 23ms indicates dropout at 44.1kHz)
        double dropoutThreshold = 23000.0; // 23ms in microseconds
        for (double time : report.processingTimes) {
            if (time > dropoutThreshold) {
                report.dropoutDetected = true;
                break;
            }
        }

        printTestReport(report);
        return report;
    }

    void printTestReport(const PerformanceReport& report) {
        std::cout << "\n--- Test Report: " << report.scenarioName << " ---" << std::endl;
        std::cout << "Total Timeouts: " << report.totalTimeouts << std::endl;
        std::cout << "Loop Preventions: " << report.totalLoopPreventions << std::endl;
        std::cout << "Emergency Bypasses: " << report.emergencyBypassCount << std::endl;
        std::cout << "Avg Processing Time: " << std::fixed << std::setprecision(2)
                  << report.avgProcessingTimeUs << " μs" << std::endl;
        std::cout << "Max Processing Time: " << std::fixed << std::setprecision(2)
                  << report.maxProcessingTimeUs << " μs" << std::endl;
        std::cout << "Dropout Detected: " << (report.dropoutDetected ? "YES" : "NO") << std::endl;

        if (report.dropoutDetected || report.totalTimeouts > 0 || report.emergencyBypassCount > 0) {
            std::cout << "*** PERFORMANCE ISSUES DETECTED ***" << std::endl;
        }
    }
};

} // namespace WaterStick

// Main test program
int main() {
    std::cout << "=== WaterStick Pitch Shifting Dropout Investigation ===" << std::endl;
    std::cout << "Test Program v1.0 - Phase 1 Completion" << std::endl;
    std::cout << "Testing scenarios for 2-5 second dropout reproduction\n" << std::endl;

    WaterStick::PitchShiftDropoutTester tester;

    try {
        auto reports = tester.runAllTests();

        // Generate summary report
        std::cout << "\n=== OVERALL SUMMARY ===" << std::endl;
        int totalIssues = 0;
        for (const auto& report : reports) {
            bool hasIssues = report.dropoutDetected || report.totalTimeouts > 0 || report.emergencyBypassCount > 0;
            if (hasIssues) {
                totalIssues++;
                std::cout << "⚠️  " << report.scenarioName << ": Issues detected" << std::endl;
            } else {
                std::cout << "✅ " << report.scenarioName << ": No issues" << std::endl;
            }
        }

        if (totalIssues > 0) {
            std::cout << "\n🔴 DROPOUTS REPRODUCED: " << totalIssues << "/" << reports.size() << " test scenarios showed problems" << std::endl;
            std::cout << "Recommend proceeding to architectural redesign phase." << std::endl;
        } else {
            std::cout << "\n🟢 NO DROPOUTS DETECTED: All test scenarios completed without issues" << std::endl;
            std::cout << "Consider testing with different parameters or longer durations." << std::endl;
        }

        // Save detailed report to file
        std::ofstream reportFile("/Users/why/repos/waterstick/pitch_dropout_test_report.txt");
        if (reportFile.is_open()) {
            reportFile << "WaterStick Pitch Shifting Dropout Test Report\n";
            reportFile << "Generated: " << __DATE__ << " " << __TIME__ << "\n\n";

            for (const auto& report : reports) {
                reportFile << "Test: " << report.scenarioName << "\n";
                reportFile << "  Timeouts: " << report.totalTimeouts << "\n";
                reportFile << "  Loop Preventions: " << report.totalLoopPreventions << "\n";
                reportFile << "  Emergency Bypasses: " << report.emergencyBypassCount << "\n";
                reportFile << "  Avg Processing: " << report.avgProcessingTimeUs << " μs\n";
                reportFile << "  Max Processing: " << report.maxProcessingTimeUs << " μs\n";
                reportFile << "  Dropout Detected: " << (report.dropoutDetected ? "YES" : "NO") << "\n\n";
            }
            reportFile.close();
            std::cout << "\nDetailed report saved to: pitch_dropout_test_report.txt" << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Test execution failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}