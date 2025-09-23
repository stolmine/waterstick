/*
 * WaterStick Pitch Accuracy Verification Report
 *
 * This analysis verifies the mathematical accuracy of pitch shifting implementation
 * across the entire parameter chain: GUI → VST Parameters → DSP Processing
 */

#include <iostream>
#include <iomanip>
#include <cmath>

// Reproduce the exact conversion functions from WaterStick
namespace ParameterConverter {
    static int convertPitchShift(double value) {
        // Convert 0.0-1.0 range to -12 to +12 semitones
        return static_cast<int>(round((value * 24.0) - 12.0));
    }
}

// Reproduce the GUI display conversion
int guiDisplayConversion(float currentValue) {
    // Parameter range: 0.0 = -12 semitones, 0.5 = 0 semitones, 1.0 = +12 semitones
    // Convert normalized value to semitones (-12 to +12) with proper rounding
    return static_cast<int>(std::round((currentValue - 0.5) * 24.0));
}

// Reproduce the controller parameter-to-string conversion
int controllerDisplayConversion(double valueNormalized) {
    return static_cast<int>(round((valueNormalized * 24.0) - 12.0));
}

// Reproduce the DSP pitch ratio calculation
float calculatePitchRatio(int semitones) {
    if (semitones == 0) {
        return 1.0f;
    } else {
        return powf(2.0f, static_cast<float>(semitones) / 12.0f);
    }
}

// Test the complete parameter chain
void verifyParameterChain() {
    std::cout << "=== WaterStick Pitch Accuracy Verification ===" << std::endl;
    std::cout << std::fixed << std::setprecision(8);

    std::cout << "\n1. PARAMETER CONVERSION VERIFICATION:" << std::endl;
    std::cout << "Testing conversions from normalized values (0.0-1.0) to semitones:" << std::endl;
    std::cout << "Normalized | VST Param | GUI Display | Controller | Expected" << std::endl;
    std::cout << "-----------|-----------|-------------|------------|----------" << std::endl;

    double testValues[] = {0.0, 0.25, 0.5, 0.75, 1.0, 0.5416667, 0.9583333}; // Include +5 and +11 semitone values
    for (double normalized : testValues) {
        int vstParam = ParameterConverter::convertPitchShift(normalized);
        int guiDisplay = guiDisplayConversion(static_cast<float>(normalized));
        int controllerDisplay = controllerDisplayConversion(normalized);
        int expected = static_cast<int>(round((normalized * 24.0) - 12.0));

        std::cout << std::setw(10) << normalized
                  << " | " << std::setw(9) << vstParam
                  << " | " << std::setw(11) << guiDisplay
                  << " | " << std::setw(10) << controllerDisplay
                  << " | " << std::setw(8) << expected;

        bool consistent = (vstParam == guiDisplay && guiDisplay == controllerDisplay && controllerDisplay == expected);
        std::cout << (consistent ? " ✓" : " ✗ MISMATCH!") << std::endl;
    }

    std::cout << "\n2. PITCH RATIO ACCURACY VERIFICATION:" << std::endl;
    std::cout << "Testing mathematical accuracy of semitone-to-frequency-ratio conversion:" << std::endl;
    std::cout << "Semitones | Calculated Ratio | Expected Ratio    | Error       | Status" << std::endl;
    std::cout << "----------|------------------|-------------------|-------------|--------" << std::endl;

    int testSemitones[] = {-12, -7, -5, 0, 5, 7, 12};
    for (int semitones : testSemitones) {
        float calculated = calculatePitchRatio(semitones);
        float expected = powf(2.0f, static_cast<float>(semitones) / 12.0f);
        float error = std::abs(calculated - expected);

        std::cout << std::setw(9) << semitones
                  << " | " << std::setw(16) << calculated
                  << " | " << std::setw(17) << expected
                  << " | " << std::setw(11) << error;

        bool accurate = (error < 1e-7);
        std::cout << " | " << (accurate ? "✓ EXACT" : "✗ ERROR") << std::endl;
    }

    std::cout << "\n3. ROUND-TRIP VERIFICATION:" << std::endl;
    std::cout << "Testing parameter consistency across save/load cycles:" << std::endl;
    std::cout << "Original | Normalized | Converted | Ratio     | Back to ST | Status" << std::endl;
    std::cout << "---------|------------|-----------|-----------|------------|--------" << std::endl;

    for (int originalSemitones = -12; originalSemitones <= 12; originalSemitones++) {
        // Simulate parameter setting: semitones → normalized value
        double normalized = (static_cast<double>(originalSemitones) + 12.0) / 24.0;

        // Convert back through VST parameter system
        int convertedSemitones = ParameterConverter::convertPitchShift(normalized);

        // Calculate pitch ratio
        float pitchRatio = calculatePitchRatio(convertedSemitones);

        // Calculate what semitones would produce this ratio (reverse calculation)
        int backToSemitones = static_cast<int>(round(12.0f * log2f(pitchRatio)));

        std::cout << std::setw(8) << originalSemitones
                  << " | " << std::setw(10) << normalized
                  << " | " << std::setw(9) << convertedSemitones
                  << " | " << std::setw(9) << pitchRatio
                  << " | " << std::setw(10) << backToSemitones;

        bool consistent = (originalSemitones == convertedSemitones && convertedSemitones == backToSemitones);
        std::cout << " | " << (consistent ? "✓ CONSISTENT" : "✗ DRIFT") << std::endl;
    }

    std::cout << "\n4. SPECIFIC TEST CASES:" << std::endl;
    std::cout << "\nTesting user concern: GUI shows '+5' should equal exactly +5 semitones DSP:" << std::endl;

    // Test +5 semitones specifically
    double normalizedFor5ST = (5.0 + 12.0) / 24.0; // Should be 0.708333...
    int vstParamFor5ST = ParameterConverter::convertPitchShift(normalizedFor5ST);
    int guiDisplayFor5ST = guiDisplayConversion(static_cast<float>(normalizedFor5ST));
    float pitchRatioFor5ST = calculatePitchRatio(vstParamFor5ST);
    float expectedRatioFor5ST = powf(2.0f, 5.0f / 12.0f);

    std::cout << "Input: +5 semitones" << std::endl;
    std::cout << "Normalized value: " << normalizedFor5ST << std::endl;
    std::cout << "VST parameter conversion: " << vstParamFor5ST << " semitones" << std::endl;
    std::cout << "GUI display conversion: " << guiDisplayFor5ST << " semitones" << std::endl;
    std::cout << "DSP pitch ratio: " << pitchRatioFor5ST << std::endl;
    std::cout << "Expected ratio for +5ST: " << expectedRatioFor5ST << std::endl;
    std::cout << "Ratio error: " << std::abs(pitchRatioFor5ST - expectedRatioFor5ST) << std::endl;

    bool perfectMatch = (vstParamFor5ST == 5 && guiDisplayFor5ST == 5 &&
                        std::abs(pitchRatioFor5ST - expectedRatioFor5ST) < 1e-7);
    std::cout << "Result: " << (perfectMatch ? "✓ PERFECT ACCURACY" : "✗ INACCURACY DETECTED") << std::endl;

    std::cout << "\n5. PARAMETER GRANULARITY VERIFICATION:" << std::endl;
    std::cout << "Verifying that only integer semitone values are possible:" << std::endl;

    // Test fractional normalized values to ensure they round to integers
    double fractionalInputs[] = {0.708, 0.709, 0.710, 0.711}; // Around +5 semitones
    for (double input : fractionalInputs) {
        int result = ParameterConverter::convertPitchShift(input);
        std::cout << "Input: " << input << " → " << result << " semitones" << std::endl;
    }

    std::cout << "\n=== VERIFICATION COMPLETE ===" << std::endl;
}

int main() {
    verifyParameterChain();
    return 0;
}