#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace hardtune
{

// Pitch-synchronous granular formant shifter (PSOLA-style).
//
// Grains two periods long are taken one period apart, resampled by the
// formant ratio, Hann-windowed and overlap-added at the *original* period.
// The harmonic spacing (perceived pitch) stays put while the spectral
// envelope (formants / vocal character) scales by the ratio: > 1 squeezes
// the voice smaller and brighter, < 1 makes it deeper and darker.
//
// Needs to be told the current fundamental so the grains stay
// pitch-synchronous — in this plugin that comes from the tuner's target
// note, which the tuned vocal is already sitting on.
class FormantShifter
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;

        int size = 1;
        while (size < (int) (sr * 0.25))
            size <<= 1;
        mask = size - 1;
        buffer.assign ((size_t) size, 0.0f);

        writeCount  = 0;
        nextGrainAt = 0;
        period      = sr / 200.0;
        for (auto& g : grains)
            g.active = false;
    }

    // Applied immediately (zero glide, same as the tuner).
    void setRatio (double newRatio) noexcept
    {
        ratio = std::clamp (newRatio, 0.5, 2.0);
    }

    void setFundamental (double hz) noexcept
    {
        if (hz > 0.0)
            period = std::clamp (sr / hz, sr / 1000.0, sr / 60.0);
    }

    float processSample (float input) noexcept
    {
        buffer[(size_t) (writeCount & (int64_t) mask)] = input;

        if (writeCount >= nextGrainAt)
        {
            spawnGrain();
            nextGrainAt = writeCount + std::max<int64_t> (1, (int64_t) std::llround (period));
        }

        float out = 0.0f;
        for (auto& g : grains)
        {
            if (! g.active)
                continue;

            const int64_t pos = writeCount - g.startOutput;
            if (pos >= g.length)
            {
                g.active = false;
                continue;
            }

            // Hann grains, two periods long, one period apart: the window
            // overlap sums to exactly 1 while the period holds steady.
            const double w = 0.5 * (1.0 - std::cos (twoPi * (double) pos / (double) g.length));
            out += (float) (w * readInterpolated ((double) g.sourceStart + (double) pos * ratio));
        }

        ++writeCount;
        return out;
    }

    // Keeps the buffer fed while the stage is out of the signal path so
    // engaging it doesn't replay stale audio.
    void skipSample (float input) noexcept
    {
        buffer[(size_t) (writeCount & (int64_t) mask)] = input;
        ++writeCount;
    }

private:
    struct Grain
    {
        int64_t startOutput = 0;
        int64_t sourceStart = 0;
        int64_t length      = 0;
        bool    active      = false;
    };

    void spawnGrain() noexcept
    {
        for (auto& g : grains)
        {
            if (g.active)
                continue;

            g.active      = true;
            g.startOutput = writeCount;
            g.length      = std::max<int64_t> (8, (int64_t) std::llround (2.0 * period));
            // The widest resample ratio is 2, so a 2x-length margin keeps
            // every grain read strictly in the past.
            g.sourceStart = writeCount - 2 * g.length;
            return;
        }
    }

    float readInterpolated (double position) const noexcept
    {
        const auto index = (int64_t) std::floor (position);
        const float frac = (float) (position - (double) index);

        const float a = buffer[(size_t) (index & (int64_t) mask)];
        const float b = buffer[(size_t) ((index + 1) & (int64_t) mask)];
        return a + frac * (b - a);
    }

    static constexpr double twoPi = 6.28318530717958647692;

    std::vector<float> buffer;
    int mask = 0;
    int64_t writeCount = 0, nextGrainAt = 0;
    double sr = 44100.0, period = 220.0, ratio = 1.0;
    Grain grains[8];
};

} // namespace hardtune
