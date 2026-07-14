#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace hardtune
{

// Fixed-time feedback echo with self-ducking: while the (dry) vocal is
// present the wet repeats pull back out of the way, and when the vocal
// pauses they bloom into the gap. Fills the blanks instead of stacking
// mud under the performance. The wet amount acts as the activator: at 0
// the effect is fully out of the path.
class Echo
{
public:
    void prepare (double sampleRate)
    {
        delaySamples = std::max (1, (int) std::lround (sampleRate * delaySeconds));
        for (auto& b : buffers)
            b.assign ((size_t) delaySamples, 0.0f);
        writePos = 0;

        envelope = 0.0f;
        envelopeRelease = (float) std::exp (-1.0 / (envReleaseSeconds * sampleRate));
    }

    void reset()
    {
        for (auto& b : buffers)
            std::fill (b.begin(), b.end(), 0.0f);
        envelope = 0.0f;
    }

    void processMono (float* samples, int numSamples, float wet) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float delayed = buffers[0][(size_t) writePos];
            buffers[0][(size_t) writePos] = samples[i] + delayed * feedback;
            samples[i] += wet * delayed * duckGain (samples[i]);
            if (++writePos >= delaySamples)
                writePos = 0;
        }
    }

    void processStereo (float* left, float* right, int numSamples, float wet) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float delayedL = buffers[0][(size_t) writePos];
            const float delayedR = buffers[1][(size_t) writePos];
            buffers[0][(size_t) writePos] = left[i] + delayedL * feedback;
            buffers[1][(size_t) writePos] = right[i] + delayedR * feedback;

            const float duck = duckGain (std::max (std::abs (left[i]), std::abs (right[i])));
            left[i]  += wet * delayedL * duck;
            right[i] += wet * delayedR * duck;
            if (++writePos >= delaySamples)
                writePos = 0;
        }
    }

private:
    // Instant-attack / ~120 ms-release envelope on the dry input drives the
    // duck: singing pushes the repeats down to 20%, silence lets them back.
    float duckGain (float input) noexcept
    {
        const float level = std::abs (input);
        envelope = level > envelope ? level : envelope * envelopeRelease;
        const float drive = std::min (1.0f, envelope * (1.0f / duckThreshold));
        return 1.0f - duckDepth * drive;
    }

    static constexpr double delaySeconds      = 0.375;
    static constexpr float  feedback          = 0.38f;
    static constexpr float  duckDepth         = 0.8f;
    static constexpr float  duckThreshold     = 0.03f; // ~ -30 dBFS
    static constexpr double envReleaseSeconds = 0.12;

    std::vector<float> buffers[2];
    int delaySamples = 1, writePos = 0;
    float envelope = 0.0f, envelopeRelease = 0.0f;
};

} // namespace hardtune
