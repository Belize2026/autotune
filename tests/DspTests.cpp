// Headless tests for the HardTune DSP chain. Pure C++ (no JUCE), so this
// builds and runs anywhere: detection accuracy, quantiser snapping, and an
// end-to-end detect -> quantise -> shift -> re-detect round trip.

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "dsp/Echo.h"
#include "dsp/FormantShifter.h"
#include "dsp/PitchDetector.h"
#include "dsp/PitchShifter.h"
#include "dsp/Quantizer.h"

namespace
{
int failures = 0;

void check (bool condition, const std::string& name, const std::string& detail = {})
{
    if (condition)
    {
        std::printf ("  PASS  %s\n", name.c_str());
    }
    else
    {
        ++failures;
        std::printf ("  FAIL  %s %s\n", name.c_str(),
                     detail.empty() ? "" : ("(" + detail + ")").c_str());
    }
}

std::vector<float> makeSine (double freq, double sampleRate, double seconds, float amp = 0.5f)
{
    std::vector<float> out ((size_t) (sampleRate * seconds));
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = amp * (float) std::sin (2.0 * 3.14159265358979323846 * freq * (double) i / sampleRate);
    return out;
}

void testPitchDetection()
{
    std::printf ("Pitch detection (YIN):\n");

    for (double sampleRate : { 44100.0, 48000.0 })
    {
        for (double freq : { 110.0, 220.0, 440.0, 660.0, 880.0 })
        {
            hardtune::PitchDetector detector;
            detector.prepare (sampleRate);

            auto sine = makeSine (freq, sampleRate, 0.2);
            detector.push (sine.data(), (int) sine.size());
            const auto result = detector.detect();

            const double error = result.voiced
                                     ? std::abs (result.frequencyHz - freq) / freq
                                     : 1.0;
            check (result.voiced && error < 0.01,
                   "detect " + std::to_string ((int) freq) + " Hz @ "
                       + std::to_string ((int) sampleRate),
                   "got " + std::to_string (result.frequencyHz) + " Hz");
        }
    }

    // Silence must be reported as unvoiced.
    hardtune::PitchDetector detector;
    detector.prepare (44100.0);
    std::vector<float> silence (8192, 0.0f);
    detector.push (silence.data(), (int) silence.size());
    check (! detector.detect().voiced, "silence is unvoiced");
}

void testQuantizer()
{
    std::printf ("Quantiser:\n");
    hardtune::Quantizer q;

    // Chromatic: everything snaps to the nearest semitone.
    q.set (0, hardtune::Scale::chromatic);
    check (std::abs (q.snapFrequencyHz (440.0f) - 440.0f) < 0.01f, "chromatic A4 stays A4");
    check (std::abs (q.snapFrequencyHz (450.0f) - 440.0f) < 0.01f, "chromatic 450 Hz -> A4");
    check (std::abs (q.snapFrequencyHz (455.0f) - 466.16f) < 0.1f, "chromatic 455 Hz -> A#4");

    // C major: A#4 territory (468 Hz ~ midi 70.07) must snap out to B4.
    q.set (0, hardtune::Scale::major);
    check (std::abs (q.snapFrequencyHz (468.0f) - 493.88f) < 0.1f, "C major 468 Hz -> B4");
    check (std::abs (q.snapFrequencyHz (440.0f) - 440.0f) < 0.01f, "C major A4 stays A4");

    // A minor pentatonic (A C D E G): B4 (~493.9 Hz, midi 71) is not in the
    // scale and must snap to C5 (midi 72).
    q.set (9, hardtune::Scale::minorPentatonic);
    check (q.snapMidi (71.2f) == 72, "A min pent B4 -> C5");
    check (q.snapMidi (69.0f) == 69, "A min pent A4 stays A4");
    check (q.snapMidi (66.0f) == 67, "A min pent F#4 -> G4");

    // E minor: F4 (midi 65) is not in the scale; nearest are E (64) / F# (66).
    q.set (4, hardtune::Scale::minor);
    check (q.snapMidi (65.4f) == 66, "E minor 65.4 -> F#4");
    check (q.snapMidi (64.6f) == 64, "E minor 64.6 -> E4");
}

void testEndToEnd()
{
    std::printf ("End-to-end (detect -> quantise -> shift -> re-detect):\n");

    const double sampleRate = 44100.0;

    // 30 cents sharp of A4 — the chain should land it exactly on 440.
    const double inputFreq = 440.0 * std::pow (2.0, 30.0 / 1200.0);
    auto sine = makeSine (inputFreq, sampleRate, 0.5);

    hardtune::PitchDetector detector, verifier;
    hardtune::Quantizer quantizer;
    hardtune::PitchShifter shifter;
    detector.prepare (sampleRate);
    verifier.prepare (sampleRate);
    shifter.prepare (sampleRate);
    quantizer.set (0, hardtune::Scale::chromatic);

    std::vector<float> out (sine.size(), 0.0f);
    const int block = 256;
    for (size_t start = 0; start < sine.size(); start += (size_t) block)
    {
        const int n = (int) std::min ((size_t) block, sine.size() - start);
        detector.push (sine.data() + start, n);

        const auto r = detector.detect();
        if (r.voiced)
            shifter.setRatio (quantizer.snapFrequencyHz (r.frequencyHz) / r.frequencyHz);

        for (int i = 0; i < n; ++i)
            out[start + (size_t) i] = shifter.processSample (sine[start + (size_t) i]);
    }

    // Verify the pitch of the last stretch of output (past warm-up).
    verifier.push (out.data() + out.size() / 2, (int) (out.size() / 2));
    const auto result = verifier.detect();

    const double error = result.voiced ? std::abs (result.frequencyHz - 440.0) / 440.0 : 1.0;
    check (result.voiced && error < 0.02,
           "445.6 Hz sharp input corrected to A4",
           "got " + std::to_string (result.frequencyHz) + " Hz");

    // Output level should be in the same ballpark as the input (no blow-ups).
    float peak = 0.0f;
    for (size_t i = out.size() / 2; i < out.size(); ++i)
        peak = std::max (peak, std::abs (out[i]));
    check (peak > 0.25f && peak < 0.75f, "output level sane",
           "peak " + std::to_string (peak));
}
void testFormantShifter()
{
    std::printf ("Formant shifter (pitch must stay put):\n");

    const double sampleRate = 44100.0;
    const double freq = 220.0;

    for (double ratio : { 0.75, 1.0, 1.5 })
    {
        hardtune::FormantShifter shifter;
        shifter.prepare (sampleRate);
        shifter.setFundamental (freq);
        shifter.setRatio (ratio);

        auto sine = makeSine (freq, sampleRate, 0.6);
        std::vector<float> out (sine.size(), 0.0f);
        for (size_t i = 0; i < sine.size(); ++i)
            out[i] = shifter.processSample (sine[i]);

        hardtune::PitchDetector verifier;
        verifier.prepare (sampleRate);
        verifier.push (out.data() + out.size() / 2, (int) (out.size() / 2));
        const auto result = verifier.detect();

        const double error = result.voiced ? std::abs (result.frequencyHz - freq) / freq : 1.0;
        check (result.voiced && error < 0.02,
               "ratio " + std::to_string (ratio) + " keeps 220 Hz",
               "got " + std::to_string (result.frequencyHz) + " Hz");

        float peak = 0.0f;
        for (size_t i = out.size() / 2; i < out.size(); ++i)
            peak = std::max (peak, std::abs (out[i]));
        check (peak > 0.1f && peak < 1.0f,
               "ratio " + std::to_string (ratio) + " level sane",
               "peak " + std::to_string (peak));
    }
}

void testCronkMapping()
{
    std::printf ("Cronk mapping (0%% soft -> 100%% extreme, log-spaced):\n");
    using PS = hardtune::PitchShifter;

    check (std::abs (PS::cronkToWindowSeconds (0.0) - 0.04) < 1e-9, "0%% -> 40 ms");
    check (std::abs (PS::cronkToWindowSeconds (100.0) - 0.008) < 1e-9, "100%% -> 8 ms floor");

    const double mid = PS::cronkToWindowSeconds (50.0);
    check (mid > 0.017 && mid < 0.019, "50%% -> ~18 ms",
           std::to_string (mid * 1000.0) + " ms");
    check (PS::cronkToWindowSeconds (150.0) == PS::cronkToWindowSeconds (100.0),
           "out-of-range input clamps");
}

void testExtremeSnapWindow()
{
    std::printf ("Extreme snap window (8 ms floor):\n");

    const double sampleRate = 44100.0;
    hardtune::PitchShifter shifter;
    shifter.prepare (sampleRate);
    shifter.setWindowSeconds (0.0); // dial at 0 -> clamps to the floor
    shifter.setRatio (1.5);

    auto sine = makeSine (220.0, sampleRate, 0.3);
    float peak = 0.0f;
    bool finite = true;
    for (size_t i = 0; i < sine.size(); ++i)
    {
        const float out = shifter.processSample (sine[i]);
        finite = finite && std::isfinite (out);
        if (i > sine.size() / 2)
            peak = std::max (peak, std::abs (out));
    }

    check (finite, "output stays finite");
    check (peak > 0.05f && peak < 1.5f, "output level bounded",
           "peak " + std::to_string (peak));
}

void testEcho()
{
    std::printf ("Echo (self-ducking):\n");

    const double sampleRate = 44100.0;
    const int delaySamples = (int) std::lround (sampleRate * 0.375);

    {
        // An impulse must come back at the fixed delay time.
        hardtune::Echo echo;
        echo.prepare (sampleRate);
        std::vector<float> signal ((size_t) delaySamples * 2, 0.0f);
        signal[0] = 1.0f;
        echo.processMono (signal.data(), (int) signal.size(), 0.5f);

        check (std::abs (signal[0] - 1.0f) < 1.0e-3f, "dry impulse unchanged");
        check (signal[(size_t) delaySamples] > 0.02f,
               "first repeat lands at 375 ms",
               "got " + std::to_string (signal[(size_t) delaySamples]));

        float between = 0.0f;
        for (int i = 1; i < delaySamples; ++i)
            between = std::max (between, std::abs (signal[(size_t) i]));
        check (between < 1.0e-6f, "silence between repeats");
    }

    {
        // Ducking: repeats stay out of the way while the vocal is present
        // and bloom into the gap once it stops.
        hardtune::Echo echo;
        echo.prepare (sampleRate);

        auto voiced = makeSine (220.0, sampleRate, 1.0, 0.4f);
        std::vector<float> gap ((size_t) (sampleRate * 0.6), 0.0f);

        echo.processMono (voiced.data(), (int) voiced.size(), 1.0f);
        echo.processMono (gap.data(), (int) gap.size(), 1.0f);

        float duringVoice = 0.0f;
        for (size_t i = voiced.size() / 2; i < voiced.size(); ++i)
            duringVoice = std::max (duringVoice, std::abs (voiced[i]));
        check (duringVoice < 0.55f, "repeats ducked while the vocal plays",
               "peak " + std::to_string (duringVoice));

        float duringGap = 0.0f;
        for (float v : gap)
            duringGap = std::max (duringGap, std::abs (v));
        check (duringGap > 0.15f, "repeats fill the gap after the vocal",
               "peak " + std::to_string (duringGap));
    }
}
} // namespace

int main()
{
    std::printf ("HardTune DSP tests\n==================\n");
    testPitchDetection();
    testQuantizer();
    testEndToEnd();
    testFormantShifter();
    testCronkMapping();
    testExtremeSnapWindow();
    testEcho();

    std::printf ("==================\n%s\n", failures == 0 ? "All tests passed." : "TESTS FAILED");
    return failures == 0 ? 0 : 1;
}
