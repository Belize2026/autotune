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
  Pentatonic), Reverb dial
- Reverb: a single rotary dial (0–100 %) after the tune chain. At 0 the
  reverb is fully inactive; turning it up activates it and blends in the wet
  signal. It works independently of the tune On/Off button, so the dry
  bypass can still carry reverb.

Explicitly **not** in v1: speed knob, formant toggle, wet/dry mix for the
tuner itself.

## Prebuilt plugins (drag and drop into your DAW)

Every push builds the plugins on real macOS, Windows and Linux runners via
GitHub Actions. To grab them: repo → **Actions** → latest **Build plugins**
run → **Artifacts**:

| Artifact | Contains |
| --- | --- |
| `HardTune-macOS` | `HardTune-AU-macOS.zip` (Logic Pro) + `HardTune-VST3-macOS.zip` |
| `HardTune-Windows` | `HardTune-VST3-Windows.zip` |
| `HardTune-Linux` | `HardTune-VST3-Linux.zip` |

On a Mac, unzip and drop:

- `HardTune.component` → `~/Library/Audio/Plug-Ins/Components/`
- `HardTune.vst3` → `~/Library/Audio/Plug-Ins/VST3/`

The CI bundles are ad-hoc signed, not notarised, so macOS quarantines
downloaded copies. After copying them into place, clear the flag once:

```sh
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/HardTune.component
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/HardTune.vst3
```

Then restart Logic (Settings → Plug-in Manager → Reset & Rescan if needed).

## Building

Requires CMake ≥ 3.22 and a C++17 compiler. JUCE 8.0.8 is fetched
automatically at configure time. Built plugins are copied into your user
plugin folders automatically (disable with `-DHARDTUNE_COPY_PLUGIN=OFF`).

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
| `source/PluginEditor.*` | Single-window UI: power button, key + scale dropdowns, reverb dial, live note readout |
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
