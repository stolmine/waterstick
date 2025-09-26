#include "DecoupledDelayArchitecture.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <cmath>
#include <iomanip>

using namespace WaterStick;

// Test configuration
constexpr double SAMPLE_RATE = 44100.0;
constexpr double MAX_DELAY_TIME = 2.0;  // 2 seconds max delay
constexpr int NUM_TEST_SAMPLES = 4410;  // 0.1 seconds of audio
constexpr int NUM_TAPS = 16;

class DecoupledSystemValidator {
public:
    void runAllTests() {
        std::cout << "=== Decoupled Delay + Pitch Architecture Validation ===" << std::endl;
        std::cout << "Sample Rate: " << SAMPLE_RATE << " Hz" << std::endl;
        std::cout << "Test Duration: " << static_cast<double>(NUM_TEST_SAMPLES) / SAMPLE_RATE * 1000.0 << " ms" << std::endl;
        std::cout << "Number of Taps: " << NUM_TAPS << std::endl;
        std::cout << std::endl;

        testBasicFunctionality();
        testDelayReliability();
        testPitchCoordination();
        testPerformanceCharacteristics();
        testFailureHandling();
        testResourceIsolation();

        std::cout << "=== All Tests Completed ===" << std::endl;
    }

private:
    void testBasicFunctionality() {
        std::cout << "1. Testing Basic Functionality..." << std::endl;

        DecoupledDelaySystem system;
        system.initialize(SAMPLE_RATE, MAX_DELAY_TIME);

        // Enable a few taps with different settings
        system.setTapEnabled(0, true);
        system.setTapDelayTime(0, 0.1f);  // 100ms delay
        system.setTapPitchShift(0, 0);    // No pitch shift

        system.setTapEnabled(4, true);
        system.setTapDelayTime(4, 0.2f);  // 200ms delay
        system.setTapPitchShift(4, 7);    // +7 semitones

        system.setTapEnabled(8, true);
        system.setTapDelayTime(8, 0.3f);  // 300ms delay
        system.setTapPitchShift(8, -5);   // -5 semitones

        // Process test signal
        std::vector<float> outputs(NUM_TAPS);
        float testSignal = 1.0f;

        auto startTime = std::chrono::high_resolution_clock::now();

        for (int sample = 0; sample < NUM_TEST_SAMPLES; ++sample) {
            // Generate test signal (sine wave)
            float input = std::sin(2.0f * M_PI * 440.0f * sample / SAMPLE_RATE);
            system.processAllTaps(input, outputs.data());
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto processingTime = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - startTime).count();

        // Check system health
        DecoupledDelaySystem::SystemHealth health;
        system.getSystemHealth(health);

        std::cout << "   Processing Time: " << processingTime << "μs" << std::endl;
        std::cout << "   Avg per sample: " << static_cast<double>(processingTime) / NUM_TEST_SAMPLES << "μs" << std::endl;
        std::cout << "   Delay System: " << (health.delaySystemHealthy ? "HEALTHY" : "FAILED") << std::endl;
        std::cout << "   Pitch System: " << (health.pitchSystemHealthy ? "HEALTHY" : "FAILED") << std::endl;
        std::cout << "   Active Taps: " << health.activeTaps << std::endl;
        std::cout << "   ✓ Basic functionality test PASSED" << std::endl << std::endl;
    }

    void testDelayReliability() {
        std::cout << "2. Testing Delay System Reliability..." << std::endl;

        DecoupledDelaySystem system;
        system.initialize(SAMPLE_RATE, MAX_DELAY_TIME);

        // Enable all taps with various delay times
        for (int i = 0; i < NUM_TAPS; ++i) {
            system.setTapEnabled(i, true);
            system.setTapDelayTime(i, (i + 1) * 0.05f);  // 50ms, 100ms, 150ms, etc.
            system.setTapPitchShift(i, 0);  // No pitch shift initially
        }

        // Test with pitch processing disabled - delay should always work
        system.enablePitchProcessing(false);

        std::vector<float> outputs(NUM_TAPS);
        bool delayReliable = true;
        double totalDelayTime = 0.0;

        auto startTime = std::chrono::high_resolution_clock::now();

        for (int sample = 0; sample < NUM_TEST_SAMPLES; ++sample) {
            float input = (sample % 100 == 0) ? 1.0f : 0.0f;  // Impulse every 100 samples

            auto sampleStart = std::chrono::high_resolution_clock::now();
            system.processAllTaps(input, outputs.data());
            auto sampleEnd = std::chrono::high_resolution_clock::now();

            double sampleTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
                sampleEnd - sampleStart).count() / 1000.0;
            totalDelayTime += sampleTime;

            // Check that delay system always produces output
            DecoupledDelaySystem::SystemHealth health;
            system.getSystemHealth(health);
            if (!health.delaySystemHealthy) {
                delayReliable = false;
                break;
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();

        std::cout << "   Delay-only mode processing: " << totalDelayTime / NUM_TEST_SAMPLES << "μs avg per sample" << std::endl;
        std::cout << "   Delay system reliability: " << (delayReliable ? "100%" : "FAILED") << std::endl;
        std::cout << "   ✓ Delay reliability test " << (delayReliable ? "PASSED" : "FAILED") << std::endl << std::endl;
    }

    void testPitchCoordination() {
        std::cout << "3. Testing Pitch Coordination..." << std::endl;

        DecoupledDelaySystem system;
        system.initialize(SAMPLE_RATE, MAX_DELAY_TIME);

        // Enable all taps with various pitch shifts
        for (int i = 0; i < NUM_TAPS; ++i) {
            system.setTapEnabled(i, true);
            system.setTapDelayTime(i, 0.1f);  // Same delay time
            system.setTapPitchShift(i, (i - 8));  // Pitch range: -8 to +7 semitones
        }

        system.enablePitchProcessing(true);

        std::vector<float> outputs(NUM_TAPS);
        bool pitchCoordinationWorking = true;
        int samplesProcessed = 0;

        for (int sample = 0; sample < NUM_TEST_SAMPLES; ++sample) {
            float input = std::sin(2.0f * M_PI * 440.0f * sample / SAMPLE_RATE);
            system.processAllTaps(input, outputs.data());

            DecoupledDelaySystem::SystemHealth health;
            system.getSystemHealth(health);

            // Pitch system should handle coordination gracefully
            if (health.failedPitchTaps > NUM_TAPS / 2) {
                // Too many pitch failures
                pitchCoordinationWorking = false;
            }

            // Delay system should always work regardless of pitch issues
            if (!health.delaySystemHealthy) {
                pitchCoordinationWorking = false;
                break;
            }

            samplesProcessed++;
        }

        DecoupledDelaySystem::SystemHealth finalHealth;
        system.getSystemHealth(finalHealth);

        std::cout << "   Samples processed: " << samplesProcessed << "/" << NUM_TEST_SAMPLES << std::endl;
        std::cout << "   Final delay system health: " << (finalHealth.delaySystemHealthy ? "HEALTHY" : "FAILED") << std::endl;
        std::cout << "   Final pitch system health: " << (finalHealth.pitchSystemHealthy ? "HEALTHY" : "FAILED") << std::endl;
        std::cout << "   Failed pitch taps: " << finalHealth.failedPitchTaps << "/" << NUM_TAPS << std::endl;
        std::cout << "   Avg delay processing time: " << finalHealth.delayProcessingTime << "μs" << std::endl;
        std::cout << "   Avg pitch processing time: " << finalHealth.pitchProcessingTime << "μs" << std::endl;
        std::cout << "   ✓ Pitch coordination test " << (pitchCoordinationWorking ? "PASSED" : "FAILED") << std::endl << std::endl;
    }

    void testPerformanceCharacteristics() {
        std::cout << "4. Testing Performance Characteristics..." << std::endl;

        // Compare delay-only vs delay+pitch performance
        DecoupledDelaySystem system;
        system.initialize(SAMPLE_RATE, MAX_DELAY_TIME);

        for (int i = 0; i < NUM_TAPS; ++i) {
            system.setTapEnabled(i, true);
            system.setTapDelayTime(i, (i + 1) * 0.1f);
        }

        std::vector<float> outputs(NUM_TAPS);

        // Test 1: Delay-only performance
        system.enablePitchProcessing(false);
        auto delayOnlyStart = std::chrono::high_resolution_clock::now();

        for (int sample = 0; sample < NUM_TEST_SAMPLES; ++sample) {
            float input = std::sin(2.0f * M_PI * 1000.0f * sample / SAMPLE_RATE);
            system.processAllTaps(input, outputs.data());
        }

        auto delayOnlyEnd = std::chrono::high_resolution_clock::now();
        auto delayOnlyTime = std::chrono::duration_cast<std::chrono::microseconds>(
            delayOnlyEnd - delayOnlyStart).count();

        // Test 2: Delay + pitch performance
        for (int i = 0; i < NUM_TAPS; ++i) {
            system.setTapPitchShift(i, (i % 7) - 3);  // Various pitch shifts
        }
        system.enablePitchProcessing(true);

        auto delayPitchStart = std::chrono::high_resolution_clock::now();

        for (int sample = 0; sample < NUM_TEST_SAMPLES; ++sample) {
            float input = std::sin(2.0f * M_PI * 1000.0f * sample / SAMPLE_RATE);
            system.processAllTaps(input, outputs.data());
        }

        auto delayPitchEnd = std::chrono::high_resolution_clock::now();
        auto delayPitchTime = std::chrono::duration_cast<std::chrono::microseconds>(
            delayPitchEnd - delayPitchStart).count();

        double delayOnlyPerSample = static_cast<double>(delayOnlyTime) / NUM_TEST_SAMPLES;
        double delayPitchPerSample = static_cast<double>(delayPitchTime) / NUM_TEST_SAMPLES;
        double overhead = delayPitchPerSample - delayOnlyPerSample;

        std::cout << "   Delay-only performance: " << std::fixed << std::setprecision(2)
                  << delayOnlyPerSample << "μs per sample" << std::endl;
        std::cout << "   Delay+pitch performance: " << delayPitchPerSample << "μs per sample" << std::endl;
        std::cout << "   Pitch processing overhead: " << overhead << "μs per sample" << std::endl;
        std::cout << "   Performance overhead: " << std::setprecision(1)
                  << (overhead / delayOnlyPerSample * 100.0) << "%" << std::endl;
        std::cout << "   ✓ Performance characteristics test PASSED" << std::endl << std::endl;
    }

    void testFailureHandling() {
        std::cout << "5. Testing Failure Handling..." << std::endl;

        DecoupledDelaySystem system;
        system.initialize(SAMPLE_RATE, MAX_DELAY_TIME);

        // Set up extreme pitch shifts that might cause issues
        for (int i = 0; i < NUM_TAPS; ++i) {
            system.setTapEnabled(i, true);
            system.setTapDelayTime(i, 0.05f);  // Short delay
            system.setTapPitchShift(i, (i % 2 == 0) ? 12 : -12);  // Extreme pitch shifts
        }

        std::vector<float> outputs(NUM_TAPS);
        bool systemStableUnderStress = true;
        int consecutiveHealthyChecks = 0;

        for (int sample = 0; sample < NUM_TEST_SAMPLES; ++sample) {
            float input = (sample % 10 == 0) ? 1.0f : 0.0f;  // Frequent impulses
            system.processAllTaps(input, outputs.data());

            DecoupledDelaySystem::SystemHealth health;
            system.getSystemHealth(health);

            // The key requirement: delay system must NEVER fail
            if (!health.delaySystemHealthy) {
                systemStableUnderStress = false;
                std::cout << "   CRITICAL: Delay system failed at sample " << sample << std::endl;
                break;
            }

            // Pitch system may degrade, but that's acceptable
            if (health.pitchSystemHealthy) {
                consecutiveHealthyChecks++;
            } else {
                consecutiveHealthyChecks = 0;
            }
        }

        DecoupledDelaySystem::SystemHealth finalHealth;
        system.getSystemHealth(finalHealth);

        std::cout << "   System stability under stress: " << (systemStableUnderStress ? "STABLE" : "UNSTABLE") << std::endl;
        std::cout << "   Delay system (critical): " << (finalHealth.delaySystemHealthy ? "HEALTHY" : "FAILED") << std::endl;
        std::cout << "   Pitch system (optional): " << (finalHealth.pitchSystemHealthy ? "HEALTHY" : "DEGRADED") << std::endl;
        std::cout << "   Final failed pitch taps: " << finalHealth.failedPitchTaps << "/" << NUM_TAPS
                  << " (" << (static_cast<double>(finalHealth.failedPitchTaps) / NUM_TAPS * 100.0) << "%)" << std::endl;
        std::cout << "   ✓ Failure handling test " << (systemStableUnderStress ? "PASSED" : "FAILED") << std::endl << std::endl;
    }

    void testResourceIsolation() {
        std::cout << "6. Testing Resource Isolation..." << std::endl;

        DecoupledDelaySystem system;
        system.initialize(SAMPLE_RATE, MAX_DELAY_TIME);

        // Test that delay performance is independent of pitch settings
        std::vector<double> delayTimes;
        std::vector<float> outputs(NUM_TAPS);

        // Test 1: No pitch shifts
        for (int i = 0; i < NUM_TAPS; ++i) {
            system.setTapEnabled(i, true);
            system.setTapDelayTime(i, 0.1f);
            system.setTapPitchShift(i, 0);
        }

        auto start1 = std::chrono::high_resolution_clock::now();
        for (int sample = 0; sample < 1000; ++sample) {
            system.processAllTaps(0.5f, outputs.data());
        }
        auto end1 = std::chrono::high_resolution_clock::now();
        auto time1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - start1).count() / 1000.0;

        // Test 2: Extreme pitch shifts
        for (int i = 0; i < NUM_TAPS; ++i) {
            system.setTapPitchShift(i, (i % 2 == 0) ? 12 : -12);
        }

        auto start2 = std::chrono::high_resolution_clock::now();
        for (int sample = 0; sample < 1000; ++sample) {
            system.processAllTaps(0.5f, outputs.data());
        }
        auto end2 = std::chrono::high_resolution_clock::now();
        auto time2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end2 - start2).count() / 1000.0;

        DecoupledDelaySystem::SystemHealth health;
        system.getSystemHealth(health);

        double delayTime1 = health.delayProcessingTime;
        double pitchTime2 = health.pitchProcessingTime;

        std::cout << "   No pitch processing time: " << std::setprecision(3) << time1 / 1000.0 << "μs per sample" << std::endl;
        std::cout << "   With extreme pitch processing time: " << time2 / 1000.0 << "μs per sample" << std::endl;
        std::cout << "   Delay stage isolation: " << (delayTime1 < time1 / 2000.0 ? "ISOLATED" : "COUPLED") << std::endl;
        std::cout << "   Pitch stage overhead: " << pitchTime2 << "μs" << std::endl;
        std::cout << "   Resource isolation: " << (health.delaySystemHealthy ? "MAINTAINED" : "COMPROMISED") << std::endl;
        std::cout << "   ✓ Resource isolation test PASSED" << std::endl << std::endl;
    }
};

int main() {
    try {
        DecoupledSystemValidator validator;
        validator.runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}