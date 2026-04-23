#pragma once
#include <JuceHeader.h>
#include <cmath>

namespace chainmaid
{
/*
 * Envelope-following feed-forward sidechain compressor.
 *
 *   input ─┐
 *          ├─▶ (gain = f(envelope(sidechain))) ─▶ output
 *   sidechain (internal fallback = input)
 *
 * The envelope follower is a classic one-pole attack / release tracker
 * on the rectified sidechain signal. The gain curve is a soft-knee
 * threshold/ratio downward compressor with output makeup.
 *
 * Engine is single-precision, single-channel; the processor
 * instantiates one per channel.
 */
class SideChainEngine
{
public:
    void prepare(double sampleRate) noexcept
    {
        sr = sampleRate;
        recomputeTimeConstants();
        envDb = -120.0f;
    }

    void setThresholdDb(float v) noexcept { thresholdDb = juce::jlimit(-60.0f,  0.0f,  v); }
    void setRatio      (float v) noexcept { ratio       = juce::jlimit(  1.0f, 20.0f,  v); }
    void setKneeDb     (float v) noexcept { kneeDb      = juce::jlimit(  0.0f, 24.0f,  v); }
    void setAttackMs   (float v) noexcept { attackMs    = juce::jlimit(  0.1f, 500.0f, v); recomputeTimeConstants(); }
    void setReleaseMs  (float v) noexcept { releaseMs   = juce::jlimit(  1.0f, 2000.f, v); recomputeTimeConstants(); }
    void setMakeupDb   (float v) noexcept { makeupDb    = juce::jlimit(-24.0f, 24.0f,  v); }

    // Returns current instantaneous gain in dB (for metering).
    float getLastGrDb() const noexcept { return lastGrDb; }

    // Process one sample; `sc` is the sidechain driver (sample peak
    // amplitude of the sidechain signal at this time step).
    float processSample(float input, float sc) noexcept
    {
        const float detInDb = juce::Decibels::gainToDecibels(std::abs(sc), -120.0f);

        // One-pole envelope follower in dB domain.
        const float coeff = (detInDb > envDb) ? atkCoeff : relCoeff;
        envDb = detInDb + coeff * (envDb - detInDb);

        // Soft-knee curve: the knee interpolates linearly between
        // 0 dB gain reduction below (T - K/2) and full ratio-driven
        // reduction above (T + K/2).
        const float over = envDb - thresholdDb;
        float grDb = 0.0f;
        if (kneeDb > 1.0e-3f && over > -kneeDb * 0.5f && over < kneeDb * 0.5f)
        {
            const float x = over + kneeDb * 0.5f;
            grDb = (1.0f / ratio - 1.0f) * (x * x) / (2.0f * kneeDb);
        }
        else if (over >= kneeDb * 0.5f)
        {
            grDb = (1.0f / ratio - 1.0f) * over;
        }

        lastGrDb = grDb;
        const float totalGainDb = grDb + makeupDb;
        return input * juce::Decibels::decibelsToGain(totalGainDb);
    }

private:
    void recomputeTimeConstants() noexcept
    {
        atkCoeff = (float) std::exp(-1.0 / (0.001 * attackMs  * juce::jmax(1.0, sr)));
        relCoeff = (float) std::exp(-1.0 / (0.001 * releaseMs * juce::jmax(1.0, sr)));
    }

    double sr = 44100.0;
    float thresholdDb = -18.0f, ratio = 4.0f, kneeDb = 6.0f;
    float attackMs = 5.0f, releaseMs = 120.0f, makeupDb = 0.0f;
    float atkCoeff = 0.0f, relCoeff = 0.0f;
    float envDb = -120.0f;
    float lastGrDb = 0.0f;
};
}
