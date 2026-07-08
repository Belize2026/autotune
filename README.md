# HardTune

A VST3/AU plugin that does one thing well: **instant, hard pitch snapping with
no glide**. This is the extreme robotic autotune sound used in hyperpop and
melodic rap — not a natural-sounding corrector.

## v1 scope

- Mono vocal input expected
- YIN pitch detection over a small (~23 ms) window — speed over smoothness,
  detection jitter is deliberately left in for the glitch character
- Hard quantisation to the nearest note of the selected key/scale: zero
  tolerance, zero glide, no partial correction
- Time-domain pitch shifting (dual-tap crossfaded delay line), no formant
  preservation
- Controls: On/Off, Key (C–B), Scale (Chromatic / Major / Minor / Minor
  Pentatonic)

Explicitly **not** in v1: speed knob, formant toggle, wet/dry mix.

## Building

Requires CMake ≥ 3.22 and a C++17 compiler. JUCE 8.0.8 is fetched
automatically at configure time.

### macOS (AU + VST3, for Logic Pro)

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The built plugins are copied into place automatically
(`~/Library/Audio/Plug-Ins/Components` and `~/Library/Audio/Plug-Ins/VST3`).
Then validate the AU and rescan in Logic:

```sh
auval -v aufx Htun Blze
```

If Logic doesn't pick it up straight away: Logic Pro → Settings → Plug-in
Manager → Reset & Rescan Selection.

An Xcode project is also supported: `cmake -B build -G Xcode`.

### Windows (VST3)

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

### Linux (VST3 + headless tests)

Needs the usual JUCE dev packages (`libasound2-dev libx11-dev libxext-dev
libxrandr-dev libxcursor-dev libxinerama-dev libfreetype6-dev
libfontconfig1-dev`), then build as above.

## DSP tests

A headless, JUCE-free test binary covers detection accuracy, quantiser
snapping, and an end-to-end detect → quantise → shift → re-detect round trip:

```sh
cmake --build build --target HardTuneDspTests
./build/HardTuneDspTests    # or: ctest --test-dir build
```

## Code layout

| Path | What it is |
| --- | --- |
| `source/dsp/PitchDetector.h` | YIN pitch detector (pure C++, header-only) |
| `source/dsp/Quantizer.h` | Key/scale hard quantiser (pure C++) |
| `source/dsp/PitchShifter.h` | Zero-glide delay-line pitch shifter (pure C++) |
| `source/PluginProcessor.*` | JUCE processor: parameters + the detect/quantise/shift chain |
| `source/PluginEditor.*` | Single-window UI: power button, key + scale dropdowns, live note readout |
| `tests/DspTests.cpp` | Headless DSP tests |

The DSP headers have no JUCE dependency on purpose, so the core sound can be
tested and iterated on without a DAW in the loop.

## Tuning the character

The two knobs to iterate on (in code, per the build plan) when comparing
against the Melodyne/Auto-Tune chain:

- Detection window: `sr * 0.023` in `PitchDetector::prepare` — shorter is
  twitchier and more robotic, longer is steadier.
- Shifter sweep window: `sr * 0.04` in `PitchShifter::prepare` — shorter means
  more modulation artifacts, longer smears transients.
