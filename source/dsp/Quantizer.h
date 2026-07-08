#pragma once

#include <cmath>
#include <cstdlib>

namespace hardtune
{

enum class Scale
{
    chromatic = 0,
    major,
    minor,
    minorPentatonic,
    numScales
};

// Hard pitch quantiser: snaps any frequency to the nearest note of the
// selected key/scale. Zero tolerance, zero "close enough" band — the target
// is always exactly an allowed note.
class Quantizer
{
public:
    // rootNote: 0 = C, 1 = C#, ... 11 = B
    void set (int rootNote, Scale scale) noexcept
    {
        static constexpr int majorIntervals[]  = { 0, 2, 4, 5, 7, 9, 11 };
        static constexpr int minorIntervals[]  = { 0, 2, 3, 5, 7, 8, 10 };
        static constexpr int minorPentIntervals[] = { 0, 3, 5, 7, 10 };

        for (bool& a : allowed)
            a = false;

        auto enable = [this, rootNote] (const int* intervals, int count)
        {
            for (int i = 0; i < count; ++i)
                allowed[(rootNote + intervals[i]) % 12] = true;
        };

        switch (scale)
        {
            case Scale::major:           enable (majorIntervals, 7);     break;
            case Scale::minor:           enable (minorIntervals, 7);     break;
            case Scale::minorPentatonic: enable (minorPentIntervals, 5); break;
            case Scale::chromatic:
            default:
                for (bool& a : allowed)
                    a = true;
                break;
        }
    }

    // Nearest allowed MIDI note to a fractional MIDI pitch.
    int snapMidi (float midiPitch) const noexcept
    {
        const int base = (int) std::lround (midiPitch);
        int bestNote = base;
        float bestDistance = 1.0e9f;

        // The widest gap in any supported scale is 3 semitones, so +/- 6
        // around the rounded pitch always contains an allowed note.
        for (int note = base - 6; note <= base + 6; ++note)
        {
            if (! allowed[((note % 12) + 12) % 12])
                continue;

            const float distance = std::abs ((float) note - midiPitch);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                bestNote = note;
            }
        }
        return bestNote;
    }

    float snapFrequencyHz (float frequencyHz) const noexcept
    {
        const float midi = frequencyToMidi (frequencyHz);
        return midiToFrequency ((float) snapMidi (midi));
    }

    static float frequencyToMidi (float hz) noexcept
    {
        return 69.0f + 12.0f * std::log2 (hz / 440.0f);
    }

    static float midiToFrequency (float midi) noexcept
    {
        return 440.0f * std::exp2 ((midi - 69.0f) / 12.0f);
    }

private:
    bool allowed[12] = { true, true, true, true, true, true,
                         true, true, true, true, true, true };
};

} // namespace hardtune
