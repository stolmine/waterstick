#pragma once

#include <cmath>
#include <algorithm>

namespace WaterStick {

class SVFUnit {
public:
    struct Outputs {
        double LP;  // Low pass output
        double BP;  // Band pass output
        double HP;  // High pass output
    };

    SVFUnit();
    ~SVFUnit() = default;

    void setSampleRate(double sampleRate);
    void setParameters(double frequency, double resonance);
    Outputs process(double input);
    void reset();

private:
    double sampleRate_;
    double s1_, s2_;           // Integrator states
    double g_, g1_, d_;        // Pre-computed coefficients
    double frequency_;
    double resonance_;

    void updateCoefficients();
};

class ThreeSistersFilter {
public:
    ThreeSistersFilter();
    ~ThreeSistersFilter() = default;

    void setSampleRate(double sampleRate);
    void setParameters(double frequency, double resonance, int filterType);
    double process(double input);
    void reset();

private:
    // 4 parallel filter chains (8 SVF units total)
    SVFUnit lpChain_[2];      // LP→LP for 24dB/octave lowpass
    SVFUnit hpChain_[2];      // HP→HP for 24dB/octave highpass
    SVFUnit bpChain_[2];      // LP→HP for 12dB/octave bandpass
    SVFUnit notchChain_[2];   // Parallel LP+HP mixed for notch

    // Current parameters
    double sampleRate_;
    double frequency_;
    double resonance_;
    int filterType_;
    int previousFilterType_;

    // Crossfading for smooth transitions
    double fadeProgress_;
    double fadeRate_;
    bool isTransitioning_;

    // Mix coefficients for current and previous filter types
    double currentMix_;
    double previousMix_;

    // Previous sample for continuity
    double previousOutput_;

    static constexpr double FADE_TIME_MS = 10.0; // 10ms crossfade time

    void updateFilterChains();
    double processFilterType(int type, double input);
    void startTransition(int newFilterType);
    void updateTransition();
};

} // namespace WaterStick