#include "ThreeSistersFilter.h"
#include "WaterStickParameters.h"

namespace WaterStick {

// SVF Unit Implementation
SVFUnit::SVFUnit()
    : sampleRate_(44100.0)
    , s1_(0.0)
    , s2_(0.0)
    , g_(0.0)
    , g1_(0.0)
    , d_(1.0)
    , frequency_(1000.0)
    , resonance_(0.7071) // 1/sqrt(2) for critically damped
{
    updateCoefficients();
}

void SVFUnit::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
    updateCoefficients();
}

void SVFUnit::setParameters(double frequency, double resonance) {
    frequency_ = std::max(20.0, std::min(frequency, sampleRate_ * 0.49));
    resonance_ = std::max(0.001, std::min(resonance, 50.0)); // Prevent instability
    updateCoefficients();
}

void SVFUnit::updateCoefficients() {
    // TPT (Topology Preserving Transform) method
    // Pre-warped frequency using tan(ωT/2)
    double omega = 2.0 * M_PI * frequency_ / sampleRate_;
    g_ = std::tan(omega * 0.5);

    // Compute coefficients for zero-delay feedback
    g1_ = 2.0 * resonance_ + g_;
    d_ = 1.0 / (1.0 + 2.0 * resonance_ * g_ + g_ * g_);
}

SVFUnit::Outputs SVFUnit::process(double input) {
    // Zero-delay feedback TPT SVF implementation
    double HP = (input - g1_ * s1_ - s2_) * d_;
    double v1 = g_ * HP;
    double BP = v1 + s1_;
    s1_ = BP + v1;
    double v2 = g_ * BP;
    double LP = v2 + s2_;
    s2_ = LP + v2;

    return {LP, BP, HP};
}

void SVFUnit::reset() {
    s1_ = 0.0;
    s2_ = 0.0;
}

// Three Sisters Filter Implementation
ThreeSistersFilter::ThreeSistersFilter()
    : sampleRate_(44100.0)
    , frequency_(1000.0)
    , resonance_(0.0)
    , filterType_(kFilterType_LowPass)
    , previousFilterType_(kFilterType_LowPass)
    , fadeProgress_(1.0)
    , fadeRate_(0.0)
    , isTransitioning_(false)
    , currentMix_(1.0)
    , previousMix_(0.0)
    , previousOutput_(0.0)
{
    updateFilterChains();
}

void ThreeSistersFilter::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;

    // Update fade rate based on sample rate
    double fadeSamples = (FADE_TIME_MS / 1000.0) * sampleRate_;
    fadeRate_ = 1.0 / fadeSamples;

    // Update all SVF units
    for (int i = 0; i < 2; ++i) {
        lpChain_[i].setSampleRate(sampleRate);
        hpChain_[i].setSampleRate(sampleRate);
        bpChain_[i].setSampleRate(sampleRate);
        notchChain_[i].setSampleRate(sampleRate);
    }

    updateFilterChains();
}

void ThreeSistersFilter::setParameters(double frequency, double resonance, int filterType) {
    frequency_ = frequency;
    resonance_ = resonance;

    // Check if filter type changed
    if (filterType != filterType_) {
        startTransition(filterType);
    }

    updateFilterChains();
}

void ThreeSistersFilter::updateFilterChains() {
    // Convert Three Sisters style resonance (-1.0 to +1.0) to SVF damping
    double dampingFactor;

    if (resonance_ >= 0.0) {
        // Positive resonance: reduce damping for traditional resonance
        // Map 0.0->1.0 to 0.5->0.001 (moderate damping to high resonance)
        // Start with more moderate damping to avoid total sound kill at 0
        dampingFactor = 0.5 * (1.0 - resonance_) + 0.001 * resonance_;
    } else {
        // Negative resonance: use moderate damping for clean filtering
        // We'll implement anti-resonance mixing in the process function
        dampingFactor = 0.5;
    }

    // Update all filter chains with the same parameters
    for (int i = 0; i < 2; ++i) {
        lpChain_[i].setParameters(frequency_, dampingFactor);
        hpChain_[i].setParameters(frequency_, dampingFactor);
        bpChain_[i].setParameters(frequency_, dampingFactor);
        notchChain_[i].setParameters(frequency_, dampingFactor);
    }
}

void ThreeSistersFilter::startTransition(int newFilterType) {
    if (!isTransitioning_) {
        previousFilterType_ = filterType_;
        filterType_ = newFilterType;
        fadeProgress_ = 0.0;
        isTransitioning_ = true;
    } else {
        // If already transitioning, update target but keep current progress
        filterType_ = newFilterType;
    }
}

void ThreeSistersFilter::updateTransition() {
    if (isTransitioning_) {
        fadeProgress_ += fadeRate_;

        if (fadeProgress_ >= 1.0) {
            fadeProgress_ = 1.0;
            isTransitioning_ = false;
            previousFilterType_ = filterType_;
        }

        // Calculate mix coefficients using smooth curve
        currentMix_ = fadeProgress_ * fadeProgress_ * (3.0 - 2.0 * fadeProgress_); // Smoothstep
        previousMix_ = 1.0 - currentMix_;
    } else {
        currentMix_ = 1.0;
        previousMix_ = 0.0;
    }
}

double ThreeSistersFilter::processFilterType(int type, double input) {
    switch (type) {
        case kFilterType_LowPass: {
            // LP→LP cascade for 24dB/octave lowpass
            auto stage1 = lpChain_[0].process(input);
            auto stage2 = lpChain_[1].process(stage1.LP);
            return stage2.LP;
        }

        case kFilterType_HighPass: {
            // HP→HP cascade for 24dB/octave highpass
            auto stage1 = hpChain_[0].process(input);
            auto stage2 = hpChain_[1].process(stage1.HP);
            return stage2.HP;
        }

        case kFilterType_BandPass: {
            // LP→HP cascade for 12dB/octave bandpass
            auto stage1 = bpChain_[0].process(input);
            auto stage2 = bpChain_[1].process(stage1.LP);
            return stage2.HP;
        }

        case kFilterType_Notch: {
            // Parallel LP+HP mixed for notch response
            auto lpStage1 = notchChain_[0].process(input);
            auto lpStage2 = lpChain_[1].process(lpStage1.LP);

            auto hpStage1 = notchChain_[0].process(input);
            auto hpStage2 = hpChain_[1].process(hpStage1.HP);

            return (lpStage2.LP + hpStage2.HP) * 0.5;
        }

        default:
            return input;
    }
}

double ThreeSistersFilter::process(double input) {
    updateTransition();

    double output;

    if (isTransitioning_) {
        // Crossfade between previous and current filter types
        double currentOutput = processFilterType(filterType_, input);
        double previousOutput = processFilterType(previousFilterType_, input);

        output = currentOutput * currentMix_ + previousOutput * previousMix_;
    } else {
        output = processFilterType(filterType_, input);
    }

    // Implement Three Sisters anti-resonance for negative resonance values
    if (resonance_ < 0.0) {
        double antiResonanceMix = -resonance_; // 0.0 to 1.0

        // Mix in complementary frequency content
        double complementary = 0.0;
        switch (filterType_) {
            case kFilterType_LowPass: {
                // Mix in high frequency content
                auto hpOut = hpChain_[0].process(input);
                complementary = hpOut.HP;
                break;
            }
            case kFilterType_HighPass: {
                // Mix in low frequency content
                auto lpOut = lpChain_[0].process(input);
                complementary = lpOut.LP;
                break;
            }
            case kFilterType_BandPass:
            case kFilterType_Notch:
                // For BP and Notch, mix toward dry signal
                complementary = input;
                break;
        }

        output = output * (1.0 - antiResonanceMix) + complementary * antiResonanceMix;
    }

    previousOutput_ = output;
    return output;
}

void ThreeSistersFilter::reset() {
    for (int i = 0; i < 2; ++i) {
        lpChain_[i].reset();
        hpChain_[i].reset();
        bpChain_[i].reset();
        notchChain_[i].reset();
    }

    fadeProgress_ = 1.0;
    isTransitioning_ = false;
    currentMix_ = 1.0;
    previousMix_ = 0.0;
    previousOutput_ = 0.0;
}

} // namespace WaterStick