#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace hardtune
{

// Monophonic pitch detector based on the YIN algorithm
// (de Cheveigné & Kawahara, 2002), tuned for low latency over smoothness:
// small analysis window, no temporal smoothing of any kind. Detection jitter
// is left in on purpose — it is part of the hard-tune character.
class PitchDetector
{
public:
    struct Result
    {
        float frequencyHz = 0.0f;
        float confidence  = 0.0f; // 1 = perfectly periodic
        bool  voiced      = false;
    };

    void prepare (double sampleRate,
                  float minFrequencyHz = 70.0f,
                  float maxFrequencyHz = 1000.0f)
    {
        sr     = sampleRate;
        // ~15 ms integration window: short enough to react near-instantly,
        // long enough that the estimate doesn't dissolve into per-frame
        // chatter (the processor adds a 2-frame note confirmation on top).
        window = std::max (256, (int) std::lround (sr * 0.015));
        maxLag = std::max (window / 4, (int) std::lround (sr / minFrequencyHz));
        minLag = std::max (2, (int) std::lround (sr / maxFrequencyHz));

        bufferSize = window + maxLag;
        ring.assign ((size_t) bufferSize, 0.0f);
        linear.assign ((size_t) bufferSize, 0.0f);
        cmnd.assign ((size_t) maxLag + 1, 0.0f);
        writePos = 0;
        filled   = 0;
    }

    void push (const float* samples, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            ring[(size_t) writePos] = samples[i];
            if (++writePos >= bufferSize)
                writePos = 0;
        }
        filled = std::min (filled + numSamples, bufferSize);
    }

    // Analyses the most recent (window + maxLag) samples that were pushed.
    Result detect()
    {
        Result result;
        if (filled < bufferSize)
            return result;

        // Unwrap the ring so linear[0] is the oldest sample in the window.
        for (int i = 0; i < bufferSize; ++i)
            linear[(size_t) i] = ring[(size_t) ((writePos + i) % bufferSize)];

        // Silence gate: don't chase pitch in noise-floor material.
        double energy = 0.0;
        for (int i = 0; i < bufferSize; ++i)
            energy += (double) linear[(size_t) i] * linear[(size_t) i];
        const double rms = std::sqrt (energy / (double) bufferSize);
        if (rms < silenceRms)
            return result;

        const float* x = linear.data();

        // Cumulative-mean-normalised difference function (YIN steps 2 + 3).
        double runningSum = 0.0;
        cmnd[0] = 1.0f;
        for (int tau = 1; tau <= maxLag; ++tau)
        {
            double diff = 0.0;
            for (int i = 0; i < window; ++i)
            {
                const double d = (double) x[i] - (double) x[i + tau];
                diff += d * d;
            }
            runningSum += diff;
            cmnd[(size_t) tau] = runningSum > 0.0
                                     ? (float) (diff * tau / runningSum)
                                     : 1.0f;
        }

        // Absolute-threshold search (step 4): first dip below the threshold,
        // then walk down to the bottom of that dip.
        int tauEstimate = -1;
        for (int tau = minLag; tau <= maxLag; ++tau)
        {
            if (cmnd[(size_t) tau] < threshold)
            {
                while (tau + 1 <= maxLag && cmnd[(size_t) tau + 1] < cmnd[(size_t) tau])
                    ++tau;
                tauEstimate = tau;
                break;
            }
        }

        // Aggressive fallback: if nothing dipped under the strict threshold,
        // grab the best candidate anyway unless it's clearly aperiodic. The
        // tuner should clamp onto marginal, breathy frames rather than let
        // them slip through untuned.
        if (tauEstimate < 0)
        {
            int bestTau = minLag;
            for (int tau = minLag + 1; tau <= maxLag; ++tau)
                if (cmnd[(size_t) tau] < cmnd[(size_t) bestTau])
                    bestTau = tau;

            if (cmnd[(size_t) bestTau] < fallbackThreshold)
                tauEstimate = bestTau;
        }

        if (tauEstimate < 0)
            return result; // unvoiced

        // Parabolic interpolation around the dip (step 5) for sub-sample lag.
        double betterTau = (double) tauEstimate;
        if (tauEstimate > minLag && tauEstimate < maxLag)
        {
            const double s0 = cmnd[(size_t) tauEstimate - 1];
            const double s1 = cmnd[(size_t) tauEstimate];
            const double s2 = cmnd[(size_t) tauEstimate + 1];
            const double denom = 2.0 * (2.0 * s1 - s2 - s0);
            if (std::abs (denom) > 1.0e-12)
                betterTau += (s2 - s0) / denom;
        }

        result.frequencyHz = (float) (sr / betterTau);
        result.confidence  = 1.0f - cmnd[(size_t) tauEstimate];
        result.voiced      = true;
        return result;
    }

    int getWindowSize() const noexcept { return window; }

private:
    double sr = 44100.0;
    int window = 0, minLag = 0, maxLag = 0, bufferSize = 0;
    int writePos = 0, filled = 0;

    static constexpr float threshold         = 0.15f; // YIN aperiodicity threshold
    static constexpr float fallbackThreshold = 0.35f; // grab-anyway ceiling
    static constexpr double silenceRms = 1.0e-3;      // ~ -60 dBFS gate

    std::vector<float> ring, linear, cmnd;
};

} // namespace hardtune
