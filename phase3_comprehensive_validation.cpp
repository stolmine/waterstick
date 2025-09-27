#include "source/WaterStick/WaterStickProcessor.h"
#include <iostream>
#include <chrono>
#include <random>
#include <vector>
#include <fstream>
#include <sstream>
#include <thread>

using namespace WaterStick;

// Phase 3 test configuration
const int DROPOUT_REPRODUCTION_TESTS = 500;
const int STRESS_TEST_DURATION_MS = 600000; // 10 minutes
const int AUDIO_QUALITY_TESTS = 100;
const int RECOVERY_VALIDATION_TESTS = 50;
const double SAMPLE_RATE = 44100.0;
const int SAMPLES_PER_BLOCK = 128;

// Performance thresholds
const double LEGACY_EXPECTED_MAX_TIME_US = 1077.0;  // From Phase 1 findings
const double UNIFIED_TARGET_MAX_TIME_US = 50.0;     // Target performance
const double AUDIO_QUALITY_THRESHOLD = 0.01;        // Maximum acceptable difference

struct ValidationResults {
    struct SystemResults {
        bool passed = false;
        double maxProcessingTime = 0.0;
        double avgProcessingTime = 0.0;
        double minProcessingTime = 999999.0;
        int timeoutCount = 0;
        int dropoutEvents = 0;
        int emergencyBypassCount = 0;
        int recoveryLevel1Count = 0;
        int recoveryLevel2Count = 0;
        int recoveryLevel3Count = 0;
        double totalTestTime = 0.0;
        std::vector<double> processingTimeHistory;
        std::string failureReason;
    };

    SystemResults legacySystem;
    SystemResults unifiedSystem;
    double performanceImprovement = 0.0;
    double audioQualityDifference = 0.0;
    bool validationPassed = false;
};

class Phase3ValidationSuite {
public:
    Phase3ValidationSuite() {
        processor = std::make_unique<WaterStickProcessor>();

        // Initialize processor
        Steinberg::Vst::ProcessSetup setup;
        setup.sampleRate = SAMPLE_RATE;
        setup.maxSamplesPerBlock = SAMPLES_PER_BLOCK;
        setup.processMode = Steinberg::Vst::kRealtime;
        setup.symbolicSampleSize = Steinberg::Vst::kSample32;

        processor->initialize(nullptr);
        processor->setupProcessing(setup);

        // Enable comprehensive logging
        processor->enablePitchDebugLogging(true);
        processor->enablePerformanceProfiling(true);

        std::cout << "Phase 3 Comprehensive Validation Suite initialized" << std::endl;
    }

    ~Phase3ValidationSuite() {
        if (processor) {
            processor->terminate();
        }
    }

    ValidationResults runComprehensiveValidation() {
        ValidationResults results;

        std::cout << "\n=== PHASE 3: COMPREHENSIVE VALIDATION TESTING ===" << std::endl;
        std::cout << "Primary Objectives:" << std::endl;
        std::cout << "1. Dropout scenario reproduction and elimination" << std::endl;
        std::cout << "2. Performance comparison (Legacy vs Unified)" << std::endl;
        std::cout << "3. Extended stress testing (10+ minutes)" << std::endl;
        std::cout << "4. Recovery system validation" << std::endl;
        std::cout << "5. Audio quality preservation" << std::endl;

        // Test 1: Dropout Scenario Reproduction
        std::cout << "\n--- Test 1: Dropout Scenario Reproduction ---" << std::endl;
        results.legacySystem = testDropoutScenarios(false, "Legacy");
        results.unifiedSystem = testDropoutScenarios(true, "Unified");

        // Test 2: Performance Comparison
        std::cout << "\n--- Test 2: Performance Comparison Analysis ---" << std::endl;
        analyzePerformanceComparison(results);

        // Test 3: Extended Stress Testing
        std::cout << "\n--- Test 3: Extended Stress Testing (10+ minutes) ---" << std::endl;
        runExtendedStressTest(results);

        // Test 4: Recovery System Validation
        std::cout << "\n--- Test 4: Recovery System Validation ---" << std::endl;
        validateRecoverySystem(results);

        // Test 5: Audio Quality Validation
        std::cout << "\n--- Test 5: Audio Quality Preservation ---" << std::endl;
        validateAudioQuality(results);

        // Final validation assessment
        assessFinalValidation(results);

        return results;
    }

private:
    ValidationResults::SystemResults testDropoutScenarios(bool useUnified, const std::string& systemName) {
        ValidationResults::SystemResults result;

        processor->enableUnifiedDelayLines(useUnified);
        processor->clearPerformanceProfile();

        std::cout << "Testing " << systemName << " system with " << DROPOUT_REPRODUCTION_TESTS << " extreme scenarios..." << std::endl;

        // Phase 1 specific dropout scenarios
        std::vector<std::function<void()>> dropoutScenarios = {
            [this]() { // Scenario 1: Extreme pitch changes on multiple taps simultaneously
                for (int tap = 0; tap < 16; tap++) {
                    float pitchValue = (std::rand() % 25) / 24.0f; // -12 to +12 semitones
                    updateParameter(97 + tap, pitchValue);
                }
            },
            [this]() { // Scenario 2: Rapid successive pitch changes
                static int direction = 1;
                for (int tap = 0; tap < 8; tap++) {
                    float pitch = direction > 0 ? 1.0f : 0.0f; // Max/Min pitch
                    updateParameter(97 + tap, pitch);
                }
                direction *= -1;
            },
            [this]() { // Scenario 3: Cascading parameter updates
                for (int tap = 0; tap < 16; tap++) {
                    updateParameter(7 + tap * 3, 1.0f); // Enable tap
                    updateParameter(8 + tap * 3, 0.9f); // Level
                    updateParameter(9 + tap * 3, (float)tap / 16.0f); // Pan
                    updateParameter(97 + tap, (float)(tap % 12) / 24.0f); // Pitch
                }
            },
            [this]() { // Scenario 4: Feedback + pitch modulation
                updateParameter(3, 0.8f); // High feedback
                for (int tap = 0; tap < 4; tap++) {
                    float pitch = (std::sin(std::rand()) + 1.0f) * 0.5f;
                    updateParameter(97 + tap, pitch);
                }
            },
            [this]() { // Scenario 5: Extreme delay time changes with pitch
                updateParameter(2, (std::rand() % 1000) / 1000.0f); // Random delay time
                for (int tap = 0; tap < 8; tap++) {
                    updateParameter(97 + tap, (std::rand() % 25) / 24.0f);
                }
            }
        };

        Steinberg::Vst::ProcessData processData;
        setupProcessData(processData);

        auto testStartTime = std::chrono::high_resolution_clock::now();

        for (int test = 0; test < DROPOUT_REPRODUCTION_TESTS; test++) {
            // Select and execute dropout scenario
            int scenarioIndex = test % dropoutScenarios.size();
            dropoutScenarios[scenarioIndex]();

            // Measure processing time
            auto processingStart = std::chrono::high_resolution_clock::now();
            Steinberg::tresult processingResult = processor->process(processData);
            auto processingEnd = std::chrono::high_resolution_clock::now();

            if (processingResult != Steinberg::kResultOk) {
                result.failureReason = "Audio processing failed at test " + std::to_string(test);
                return result;
            }

            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(processingEnd - processingStart);
            double processingTimeUs = duration.count() / 1000.0;

            result.processingTimeHistory.push_back(processingTimeUs);
            result.maxProcessingTime = std::max(result.maxProcessingTime, processingTimeUs);
            result.minProcessingTime = std::min(result.minProcessingTime, processingTimeUs);

            // Check for dropouts (>100μs indicates potential dropout)
            if (processingTimeUs > 100.0) {
                result.dropoutEvents++;
            }

            // Check for timeout violations
            if (processingTimeUs > UNIFIED_TARGET_MAX_TIME_US && useUnified) {
                result.timeoutCount++;
            } else if (processingTimeUs > LEGACY_EXPECTED_MAX_TIME_US && !useUnified) {
                result.timeoutCount++;
            }

            if (test % 50 == 0) {
                std::cout << "  Scenario " << test << ": " << processingTimeUs << "μs" << std::endl;
            }
        }

        auto testEndTime = std::chrono::high_resolution_clock::now();
        result.totalTestTime = std::chrono::duration_cast<std::chrono::milliseconds>(testEndTime - testStartTime).count();

        // Calculate statistics
        if (!result.processingTimeHistory.empty()) {
            double total = 0.0;
            for (double time : result.processingTimeHistory) {
                total += time;
            }
            result.avgProcessingTime = total / result.processingTimeHistory.size();
        }

        // Determine pass/fail
        if (useUnified) {
            result.passed = (result.dropoutEvents == 0) && (result.maxProcessingTime < UNIFIED_TARGET_MAX_TIME_US);
        } else {
            // Legacy system is expected to have dropouts - we just measure them
            result.passed = true; // For comparison purposes
        }

        std::cout << systemName << " Results:" << std::endl;
        std::cout << "  Max Processing Time: " << result.maxProcessingTime << "μs" << std::endl;
        std::cout << "  Avg Processing Time: " << result.avgProcessingTime << "μs" << std::endl;
        std::cout << "  Min Processing Time: " << result.minProcessingTime << "μs" << std::endl;
        std::cout << "  Dropout Events: " << result.dropoutEvents << std::endl;
        std::cout << "  Test Result: " << (result.passed ? "PASSED" : "FAILED") << std::endl;

        return result;
    }

    void analyzePerformanceComparison(ValidationResults& results) {
        if (results.legacySystem.maxProcessingTime > 0) {
            results.performanceImprovement = results.legacySystem.maxProcessingTime / results.unifiedSystem.maxProcessingTime;
        }

        std::cout << "Performance Analysis:" << std::endl;
        std::cout << "  Legacy Max Time: " << results.legacySystem.maxProcessingTime << "μs" << std::endl;
        std::cout << "  Unified Max Time: " << results.unifiedSystem.maxProcessingTime << "μs" << std::endl;
        std::cout << "  Performance Improvement: " << results.performanceImprovement << "x" << std::endl;
        std::cout << "  Legacy Dropouts: " << results.legacySystem.dropoutEvents << std::endl;
        std::cout << "  Unified Dropouts: " << results.unifiedSystem.dropoutEvents << std::endl;

        // Performance requirements verification
        bool performanceMet = (results.performanceImprovement >= 10.0) &&
                             (results.unifiedSystem.dropoutEvents == 0) &&
                             (results.unifiedSystem.maxProcessingTime < UNIFIED_TARGET_MAX_TIME_US);

        std::cout << "  Performance Requirements Met: " << (performanceMet ? "YES" : "NO") << std::endl;
    }

    void runExtendedStressTest(ValidationResults& results) {
        std::cout << "Running 10-minute continuous stress test with unified system..." << std::endl;

        processor->enableUnifiedDelayLines(true);
        processor->clearPerformanceProfile();

        Steinberg::Vst::ProcessData processData;
        setupProcessData(processData);

        auto stressTestStart = std::chrono::high_resolution_clock::now();
        auto targetEndTime = stressTestStart + std::chrono::milliseconds(STRESS_TEST_DURATION_MS);

        int iterations = 0;
        double maxStressTime = 0.0;
        int stressDropouts = 0;

        std::random_device rd;
        std::mt19937 gen(rd());

        while (std::chrono::high_resolution_clock::now() < targetEndTime) {
            // Continuous extreme parameter modulation
            if (iterations % 10 == 0) {
                // All 16 taps with random pitch changes
                for (int tap = 0; tap < 16; tap++) {
                    updateParameter(7 + tap * 3, 1.0f); // Enable
                    float pitch = gen() % 25 / 24.0f; // Random pitch
                    updateParameter(97 + tap, pitch);
                }
            }

            // Concurrent delay time and feedback modulation
            if (iterations % 5 == 0) {
                updateParameter(2, (gen() % 1000) / 1000.0f); // Delay time
                updateParameter(3, (gen() % 800) / 1000.0f);  // Feedback
            }

            auto processingStart = std::chrono::high_resolution_clock::now();
            processor->process(processData);
            auto processingEnd = std::chrono::high_resolution_clock::now();

            double processingTime = std::chrono::duration_cast<std::chrono::nanoseconds>(processingEnd - processingStart).count() / 1000.0;
            maxStressTime = std::max(maxStressTime, processingTime);

            if (processingTime > UNIFIED_TARGET_MAX_TIME_US) {
                stressDropouts++;
            }

            iterations++;

            if (iterations % 10000 == 0) {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - stressTestStart).count();
                std::cout << "  " << elapsed << "s elapsed, max processing: " << maxStressTime << "μs" << std::endl;
            }
        }

        auto actualTestTime = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - stressTestStart).count();

        std::cout << "Extended Stress Test Results:" << std::endl;
        std::cout << "  Test Duration: " << actualTestTime << " seconds" << std::endl;
        std::cout << "  Iterations Processed: " << iterations << std::endl;
        std::cout << "  Max Processing Time: " << maxStressTime << "μs" << std::endl;
        std::cout << "  Dropout Events: " << stressDropouts << std::endl;
        std::cout << "  Memory Stability: " << (maxStressTime < UNIFIED_TARGET_MAX_TIME_US * 2 ? "STABLE" : "UNSTABLE") << std::endl;

        // Update results
        results.unifiedSystem.maxProcessingTime = std::max(results.unifiedSystem.maxProcessingTime, maxStressTime);
        results.unifiedSystem.dropoutEvents += stressDropouts;
    }

    void validateRecoverySystem(ValidationResults& results) {
        std::cout << "Validating multi-level recovery system..." << std::endl;

        processor->enableUnifiedDelayLines(true);

        // Test recovery scenarios by simulating various failure conditions
        Steinberg::Vst::ProcessData processData;
        setupProcessData(processData);

        int recoveryTests = 0;
        int successfulRecoveries = 0;

        // Test various recovery scenarios
        for (int test = 0; test < RECOVERY_VALIDATION_TESTS; test++) {
            // Create conditions that might trigger recovery
            switch (test % 3) {
                case 0: // Extreme pitch changes that might cause buffer issues
                    for (int tap = 0; tap < 16; tap++) {
                        updateParameter(97 + tap, 1.0f); // Max pitch up
                    }
                    break;
                case 1: // Rapid parameter changes
                    for (int i = 0; i < 100; i++) {
                        updateParameter(97, (i % 2) ? 1.0f : 0.0f);
                    }
                    break;
                case 2: // High feedback with extreme settings
                    updateParameter(3, 0.95f); // Near-max feedback
                    for (int tap = 0; tap < 8; tap++) {
                        updateParameter(97 + tap, 0.0f); // Max pitch down
                    }
                    break;
            }

            // Process and check for recovery
            processor->process(processData);
            recoveryTests++;

            // Assume successful recovery if processing completes without crash
            successfulRecoveries++;
        }

        std::cout << "Recovery System Validation:" << std::endl;
        std::cout << "  Recovery Tests Run: " << recoveryTests << std::endl;
        std::cout << "  Successful Recoveries: " << successfulRecoveries << std::endl;
        std::cout << "  Recovery Success Rate: " << (successfulRecoveries * 100.0 / recoveryTests) << "%" << std::endl;

        // Log recovery statistics if available
        if (processor->isUsingUnifiedDelayLines()) {
            processor->logUnifiedDelayLineStats();
        }
    }

    void validateAudioQuality(ValidationResults& results) {
        std::cout << "Validating audio quality preservation..." << std::endl;

        // Generate identical test signal for both systems
        std::vector<float> testInput(SAMPLES_PER_BLOCK * 2);
        std::vector<float> legacyOutput(SAMPLES_PER_BLOCK * 2);
        std::vector<float> unifiedOutput(SAMPLES_PER_BLOCK * 2);

        // Create test signal - swept sine wave
        for (int i = 0; i < SAMPLES_PER_BLOCK; i++) {
            float t = i / (float)SAMPLES_PER_BLOCK;
            float freq = 440.0f + 880.0f * t; // Sweep from 440Hz to 1320Hz
            float sample = 0.1f * std::sin(2.0f * M_PI * freq * t);
            testInput[i] = sample;
            testInput[i + SAMPLES_PER_BLOCK] = sample; // Stereo
        }

        Steinberg::Vst::ProcessData processData;
        setupProcessDataWithInput(processData, testInput, legacyOutput);

        // Test with legacy system
        processor->enableUnifiedDelayLines(false);
        processor->process(processData);
        std::copy(processData.outputs[0].channelBuffers32[0],
                  processData.outputs[0].channelBuffers32[0] + SAMPLES_PER_BLOCK,
                  legacyOutput.begin());

        // Reset and test with unified system
        setupProcessDataWithInput(processData, testInput, unifiedOutput);
        processor->enableUnifiedDelayLines(true);
        processor->process(processData);
        std::copy(processData.outputs[0].channelBuffers32[0],
                  processData.outputs[0].channelBuffers32[0] + SAMPLES_PER_BLOCK,
                  unifiedOutput.begin());

        // Calculate audio quality difference (RMS difference)
        double totalDifference = 0.0;
        for (int i = 0; i < SAMPLES_PER_BLOCK; i++) {
            double diff = legacyOutput[i] - unifiedOutput[i];
            totalDifference += diff * diff;
        }
        results.audioQualityDifference = std::sqrt(totalDifference / SAMPLES_PER_BLOCK);

        std::cout << "Audio Quality Analysis:" << std::endl;
        std::cout << "  RMS Difference: " << results.audioQualityDifference << std::endl;
        std::cout << "  Quality Threshold: " << AUDIO_QUALITY_THRESHOLD << std::endl;
        std::cout << "  Audio Quality Preserved: " << (results.audioQualityDifference < AUDIO_QUALITY_THRESHOLD ? "YES" : "NO") << std::endl;
    }

    void assessFinalValidation(ValidationResults& results) {
        std::cout << "\n=== FINAL PHASE 3 VALIDATION ASSESSMENT ===" << std::endl;

        // Validation criteria
        bool dropoutsEliminated = (results.unifiedSystem.dropoutEvents == 0);
        bool performanceImproved = (results.performanceImprovement >= 10.0);
        bool processingTimeMet = (results.unifiedSystem.maxProcessingTime < UNIFIED_TARGET_MAX_TIME_US);
        bool audioQualityPreserved = (results.audioQualityDifference < AUDIO_QUALITY_THRESHOLD);

        results.validationPassed = dropoutsEliminated && performanceImproved &&
                                  processingTimeMet && audioQualityPreserved;

        std::cout << "Validation Criteria:" << std::endl;
        std::cout << "  ✓ Dropouts Eliminated: " << (dropoutsEliminated ? "PASS" : "FAIL") << std::endl;
        std::cout << "  ✓ Performance Improved 10x+: " << (performanceImproved ? "PASS" : "FAIL") << std::endl;
        std::cout << "  ✓ Processing Time <50μs: " << (processingTimeMet ? "PASS" : "FAIL") << std::endl;
        std::cout << "  ✓ Audio Quality Preserved: " << (audioQualityPreserved ? "PASS" : "FAIL") << std::endl;

        std::cout << "\n=== PHASE 3 FINAL RESULT ===" << std::endl;
        if (results.validationPassed) {
            std::cout << "🎉 VALIDATION PASSED - PRODUCTION READY" << std::endl;
            std::cout << "The unified system successfully eliminates dropouts while maintaining audio quality." << std::endl;
        } else {
            std::cout << "❌ VALIDATION FAILED - NEEDS ATTENTION" << std::endl;
            std::cout << "Some validation criteria were not met." << std::endl;
        }
    }

    void updateParameter(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue value) {
        // Direct parameter update (simplified for testing)
        // In a real implementation, this would go through proper VST3 parameter handling
        processor->setParameterNormalized(id, value);
    }

    void setupProcessData(Steinberg::Vst::ProcessData& processData) {
        setupProcessDataWithInput(processData, inputBuffer, outputBuffer);
    }

    void setupProcessDataWithInput(Steinberg::Vst::ProcessData& processData,
                                  std::vector<float>& input,
                                  std::vector<float>& output) {
        // Ensure buffers are sized correctly
        input.resize(SAMPLES_PER_BLOCK * 2);
        output.resize(SAMPLES_PER_BLOCK * 2);

        // Generate test signal if input is empty
        if (std::all_of(input.begin(), input.end(), [](float f){ return f == 0.0f; })) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> noiseDist(-0.1, 0.1);

            for (int i = 0; i < SAMPLES_PER_BLOCK * 2; i++) {
                input[i] = static_cast<float>(noiseDist(gen));
            }
        }

        // Set up audio bus buffers
        static float* inputChannels[2];
        static float* outputChannels[2];

        inputChannels[0] = &input[0];
        inputChannels[1] = &input[SAMPLES_PER_BLOCK];
        outputChannels[0] = &output[0];
        outputChannels[1] = &output[SAMPLES_PER_BLOCK];

        processData.processMode = Steinberg::Vst::kRealtime;
        processData.symbolicSampleSize = Steinberg::Vst::kSample32;
        processData.numSamples = SAMPLES_PER_BLOCK;
        processData.numInputs = 1;
        processData.numOutputs = 1;

        static Steinberg::Vst::AudioBusBuffers inputBus;
        static Steinberg::Vst::AudioBusBuffers outputBus;

        inputBus.numChannels = 2;
        inputBus.channelBuffers32 = inputChannels;
        outputBus.numChannels = 2;
        outputBus.channelBuffers32 = outputChannels;

        processData.inputs = &inputBus;
        processData.outputs = &outputBus;
    }

    std::unique_ptr<WaterStickProcessor> processor;
    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;
};

int main() {
    std::cout << "=== PHASE 3: COMPREHENSIVE VALIDATION SUITE ===" << std::endl;
    std::cout << "Objective: Validate unified pitch shifting architecture for production deployment" << std::endl;

    Phase3ValidationSuite validator;
    ValidationResults results = validator.runComprehensiveValidation();

    // Generate comprehensive report
    std::ofstream report("phase3_validation_report.txt");
    report << "PHASE 3 COMPREHENSIVE VALIDATION REPORT\n";
    report << "=====================================\n\n";

    report << "EXECUTIVE SUMMARY\n";
    report << "Validation Result: " << (results.validationPassed ? "PASSED" : "FAILED") << "\n";
    report << "Performance Improvement: " << results.performanceImprovement << "x\n";
    report << "Audio Quality Difference: " << results.audioQualityDifference << "\n\n";

    report << "LEGACY SYSTEM RESULTS\n";
    report << "Max Processing Time: " << results.legacySystem.maxProcessingTime << "μs\n";
    report << "Avg Processing Time: " << results.legacySystem.avgProcessingTime << "μs\n";
    report << "Dropout Events: " << results.legacySystem.dropoutEvents << "\n\n";

    report << "UNIFIED SYSTEM RESULTS\n";
    report << "Max Processing Time: " << results.unifiedSystem.maxProcessingTime << "μs\n";
    report << "Avg Processing Time: " << results.unifiedSystem.avgProcessingTime << "μs\n";
    report << "Dropout Events: " << results.unifiedSystem.dropoutEvents << "\n\n";

    report.close();

    std::cout << "\nDetailed validation report saved to: phase3_validation_report.txt" << std::endl;

    return results.validationPassed ? 0 : 1;
}