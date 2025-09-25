#pragma once

#include <vector>
#include "AdaptiveSmoother.h"

namespace WaterStick {

static const int MAX_TAPS = 64;
static const int kNumCombPatterns = 8;
static const int kNumCombSlopes = 4;

class CombProcessor
{
public:
    CombProcessor();
    ~CombProcessor();

    void initialize(double sampleRate, double maxDelaySeconds);
    void reset();

    // Parameter setters
    void setSize(float sizeSeconds);
    void setNumTaps(int numTaps);
    void setFeedback(float feedback);
    void setSyncMode(bool synced);
    void setClockDivision(int division);
    void setPitchCV(float cv);
    void setPattern(int pattern);
    void setSlope(int slope);
    void setGain(float gain);
    void setSmoothingTimeConstant(float timeConstant);

    // Tempo sync
    void updateTempo(double hostTempo, bool isValid);

    // Adaptive smoothing controls
    void setAdaptiveSmoothingEnabled(bool enabled);
    void setCascadedSmoothingEnabled(bool enabled);
    void setAdaptiveSmoothingParameters(float combSizeSensitivity,
                                       float pitchCVSensitivity,
                                       float fastTimeConstant,
                                       float slowTimeConstant);
    void getAdaptiveSmoothingStatus(bool& enabled,
                                   float& combSizeTimeConstant,
                                   float& pitchCVTimeConstant,
                                   float& combSizeVelocity,
                                   float& pitchCVVelocity) const;
    void setEnhancedSmoothingEnabled(bool enabled);
    void setComplexityMode(int complexityMode);
    bool isEnhancedSmoothingEnabled() const;

    // Processing
    void processStereo(float inputL, float inputR, float& outputL, float& outputR);

    // Getters
    float getSyncedCombSize() const;
    float getFeedback() const { return mFeedback; }
    int getNumTaps() const { return mNumActiveTaps; }
    float getGain() const { return mGain; }

private:
    void updateSmoothingCoeff();
    void updateSmoothingCoeff(float timeConstant);
    float getTapDelay(int tapIndex, float smoothedPitchCV);
    float applyCVScaling(float baseDelay, float pitchCV);
    float getTapGain(int tapIndex) const;
    float tanhLimiter(float input) const;
    float getSmoothedCombSize();
    float getSmoothedPitchCV();

    // Internal state
    std::vector<float> mDelayBufferL;
    std::vector<float> mDelayBufferR;
    int mBufferSize;
    int mWriteIndex;
    double mSampleRate;

    // Parameters
    float mCombSize;
    int mNumActiveTaps;
    float mFeedback;
    float mPitchCV;
    float mFeedbackBufferL;
    float mFeedbackBufferR;

    // Sync parameters
    bool mIsSynced;
    int mClockDivision;
    float mHostTempo;
    bool mHostTempoValid;

    // Pattern/slope parameters
    int mPattern;
    int mSlope;
    float mGain;

    // Smoothing state
    float mPrevCombSize;
    float mAllpassState;
    float mSmoothingCoeff;
    float mSmoothingTimeConstant;
    float mPrevPitchCV;
    float mPitchAllpassState;

    // Adaptive smoothing
    bool mAdaptiveSmoothingEnabled;
    float mSmoothedCombSize;
    float mSmoothedPitchCV;
    CombParameterSmoother mAdaptiveSmoother;
};

} // namespace WaterStick