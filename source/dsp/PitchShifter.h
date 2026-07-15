#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace hardtune
{

// Time-domain pitch shifter: two delay-line taps swept at the shift rate and
// crossfaded with complementary sin^2 windows. Cheap and low latency. Ratio
// changes land within ~5 ms (a de-click ramp, not a glide) so note jumps
// sound clean but instant. Formants are not preserved (v1).
class PitchShifter
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;

        // Buffer sized for the largest window the SNAP control allows.
        int size = 1;
        while (size < (int) (sr * maxWindowSeconds) * 2)
            size <<= 1;
        mask = size - 1;
        buffer.assign ((size_t) size, 0.0f);

        window = sr * 0.026; // ~CRONK 50%, overridden by the parameter
        writeCount = 0;
        phase = 0.0;
        ratio = 1.0;
        targetRatio = 1.0;
        // Full-octave ratio change traverses in ~12 ms: the T-Pain whip.
        // Fast enough to feel instant, slow enough that the transition is
        // a clean, audible flick instead of a crunch.
        rampPerSample = 1.0 / (0.012 * sr);
    }

    // CRONK maps 0..100% onto the sweep window, log-spaced: 0% = 50 ms
    // (smooth), 100% = 14 ms (maximum usable grit). The floor is set where
    // the sound stays clean enough for professional use.
    static double cronkToWindowSeconds (double percent) noexcept
    {
        const double t = std::clamp (percent, 0.0, 100.0) / 100.0;
        return maxWindowSeconds * std::pow (minWindowSeconds / maxWindowSeconds, t);
    }

    // The crossfade/sweep window is the harshness control: shorter adds
    // metallic modulation grit and cuts latency. Corrections are instant
    // at any setting.
    void setWindowSeconds (double seconds) noexcept
    {
        window = std::clamp (seconds, minWindowSeconds, maxWindowSeconds) * sr;
        while (phase >= window)
            phase -= window;
    }

    // Effectively instant: the ~5 ms internal ramp is a de-clicker, not a
    // glide. Note jumps still sound like hard snaps.
    void setRatio (double newRatio) noexcept
    {
        targetRatio = std::clamp (newRatio, 0.25, 4.0);
    }

    float processSample (float input) noexcept
    {
        buffer[(size_t) (writeCount & (int64_t) mask)] = input;

        if (std::abs (targetRatio - ratio) > 1.0e-9)
            ratio += std::clamp (targetRatio - ratio, -rampPerSample, rampPerSample);

        // The tap delay drifts at (1 - ratio) samples per sample, i.e. the
        // taps read through the buffer at `ratio` times real time.
        phase += 1.0 - ratio;
        while (phase < 0.0)
            phase += window;
        while (phase >= window)
            phase -= window;

        const double d1 = phase;
        double d2 = phase + window * 0.5;
        if (d2 >= window)
            d2 -= window;

        // sin^2 / cos^2 crossfade: each tap is silent at its wrap point.
        const double s = std::sin (pi * d1 / window);
        const float g1 = (float) (s * s);
        const float g2 = 1.0f - g1;

        const float out = g1 * read (d1) + g2 * read (d2);
        ++writeCount;
        return out;
    }

    // Keeps the delay line fed while the plugin is bypassed so that
    // re-engaging doesn't replay stale audio.
    void skipSample (float input) noexcept
    {
        buffer[(size_t) (writeCount & (int64_t) mask)] = input;
        ++writeCount;
    }

    int getLatencySamples() const noexcept
    {
        return (int) (window * 0.5); // average tap delay
    }

private:
    float read (double delay) const noexcept
    {
        const double position = (double) writeCount - delay;
        const auto index = (int64_t) std::floor (position);
        const float frac = (float) (position - (double) index);

        const float a = buffer[(size_t) (index & (int64_t) mask)];
        const float b = buffer[(size_t) ((index + 1) & (int64_t) mask)];
        return a + frac * (b - a);
    }

    static constexpr double pi = 3.14159265358979323846;
    static constexpr double minWindowSeconds = 0.014;
    static constexpr double maxWindowSeconds = 0.05;

    std::vector<float> buffer;
    int mask = 0;
    int64_t writeCount = 0;
    double sr = 44100.0;
    double window = 0.0;
    double phase = 0.0;
    double ratio = 1.0;
    double targetRatio = 1.0;
    double rampPerSample = 0.0;
};

} // namespace hardtune
