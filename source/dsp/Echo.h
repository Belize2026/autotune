#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace hardtune
{

// Simple fixed-time feedback echo. The wet amount acts as the activator:
// at 0 the effect is fully out of the path, anything above blends the
// delayed repeats in.
class Echo
{
public:
    void prepare (double sampleRate)
    {
        delaySamples = std::max (1, (int) std::lround (sampleRate * delaySeconds));
        for (auto& b : buffers)
            b.assign ((size_t) delaySamples, 0.0f);
        writePos = 0;
    }

    void reset()
    {
        for (auto& b : buffers)
            std::fill (b.begin(), b.end(), 0.0f);
    }

    void processMono (float* samples, int numSamples, float wet) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float delayed = buffers[0][(size_t) writePos];
            buffers[0][(size_t) writePos] = samples[i] + delayed * feedback;
            samples[i] += wet * delayed;
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
            left[i]  += wet * delayedL;
            right[i] += wet * delayedR;
            if (++writePos >= delaySamples)
                writePos = 0;
        }
    }

private:
    static constexpr double delaySeconds = 0.375;
    static constexpr float  feedback     = 0.38f;

    std::vector<float> buffers[2];
    int delaySamples = 1, writePos = 0;
};

} // namespace hardtune
