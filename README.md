# HardTune

A VST3/AU plugin that does one thing well: **instant, hard pitch snapping with
no glide**. This is the extreme robotic autotune sound used in hyperpop and
melodic rap — not a natural-sounding corrector.

## Download & install

Grab the installer for your OS from the
**[latest release](https://github.com/Belize2026/autotune/releases/latest)**
(no GitHub account needed):

- **macOS**: [`HardTune-Installer-macOS.pkg`](https://github.com/Belize2026/autotune/releases/latest/download/HardTune-Installer-macOS.pkg)
- **Windows**: [`HardTune-Installer-Windows.exe`](https://github.com/Belize2026/autotune/releases/latest/download/HardTune-Installer-Windows.exe)

### macOS (Logic Pro, Ableton, etc.)

1. Download the `.pkg` above.
2. **Right-click it → Open** (double-clicking gets blocked because the
   installer is unsigned; right-click → Open shows an "Open" button instead).
   If macOS still refuses: System Settings → Privacy & Security → scroll down
   → **Open Anyway**.
3. Follow the installer (it asks for your password). It installs:
   - `HardTune.component` (AU, what Logic uses) → `/Library/Audio/Plug-Ins/Components`
   - `HardTune.vst3` → `/Library/Audio/Plug-Ins/VST3`
4. Open your DAW. In Logic the plugin appears on a channel strip under
   **Audio FX → Audio Units → Belize2026 → HardTune**. If it doesn't show up:
   Logic Pro → Settings → Plug-in Manager → **Reset & Rescan Selection**,
   then restart Logic.

### Windows

1. Download the `.exe` above.
2. If SmartScreen pops up, click **More info → Run anyway** (the installer is
   unsigned).
3. Follow the installer. It installs `HardTune.vst3` into
   `C:\Program Files\Common Files\VST3` — the standard folder every VST3 DAW
   scans.
4. Rescan plugins in your DAW (or just restart it). HardTune appears under
   Belize2026.

### Uninstalling

- **macOS**: delete `HardTune.component` and `HardTune.vst3` from the two
  folders listed above.
- **Windows**: Settings → Apps → Installed apps → HardTune → Uninstall.

### What's in this repository

This repo contains the **source code** — the compiled plugins are *not*
stored in git. GitHub Actions builds them on every push, and pushing a
version tag (e.g. `v0.1.0`) publishes the installers to the
[Releases page](https://github.com/Belize2026/autotune/releases), which is
where downloads live permanently. So: code lives here, downloadable
binaries live under Releases, and per-commit dev builds live under Actions
artifacts.

## v1 scope

- Mono vocal input expected
- YIN pitch detection over a small (~23 ms) window — speed over smoothness,
  detection jitter is deliberately left in for the glitch character
- Hard quantisation to the nearest note of the selected key/scale: zero
  tolerance, zero glide, no partial correction
- Time-domain pitch shifting (dual-tap crossfaded delay line), no formant
  preservation
- Controls: On/Off, Key (C–B), Scale (Chromatic / Major / Minor / Minor
  Pentatonic), Formant dial, Dual Vocals toggle + Dual Mix dial, Echo dial,
  Reverb dial
- Formant shift: PSOLA-style pitch-synchronous granular processing driven by
  the tuner's pitch tracking — reshapes the vocal character (±12 st) while
  the tuned pitch stays put. Two modes:
  - **Dual Vocals off** (default): the Formant dial reshapes the main vocal
    directly.
  - **Dual Vocals on**: the main vocal stays on level and a formant-shifted
    double is layered underneath, blended with the Dual Mix dial.
- Echo: a single rotary dial (0–100 %), fixed 375 ms feedback delay. At 0 it
  is fully inactive; turning it up activates it.
- Reverb: same dial-as-activator pattern (0–100 %) after the echo. Both work
  independently of the tune On/Off button, so the dry bypass can still carry
  space.
- Live correction display: a scrolling trace of the correction being applied
  (in cents, centre line = on pitch) plus the current detected → target note
  readout, so you can see exactly what the tuner is doing.

Signal chain: detect → quantise → shift → formant → echo → reverb.

Explicitly **not** in v1: speed knob, wet/dry mix for the tuner itself.

## Manual install (no installer)

Each release also carries the raw plugin bundles as zips
(`HardTune-AU-macOS.zip`, `HardTune-VST3-macOS.zip`,
`HardTune-VST3-Windows.zip`, `HardTune-VST3-Linux.zip`) if you'd rather
place the files yourself.

On a Mac, unzip and drop:

- `HardTune.component` → `~/Library/Audio/Plug-Ins/Components/`
- `HardTune.vst3` → `~/Library/Audio/Plug-Ins/VST3/`

Manually downloaded bundles get quarantined by macOS (they are ad-hoc
signed, not notarised — another reason the `.pkg` installer is the easier
route, since installed files skip quarantine). After copying them into
place, clear the flag once:

```sh
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/HardTune.component
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/HardTune.vst3
```

Then restart Logic (Settings → Plug-in Manager → Reset & Rescan if needed).

On Windows, unzip and drop `HardTune.vst3` into
`C:\Program Files\Common Files\VST3`.

## Dev builds (every push)

Every push also builds everything via GitHub Actions: repo → **Actions** →
latest **Build plugins** run → **Artifacts** (requires being signed in to
GitHub; artifacts expire after 90 days). That includes the installers, the
platform zips, and the raw Windows `HardTune.vst3` bundle as its own
artifact.

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
| `source/dsp/FormantShifter.h` | PSOLA-style formant shifter (pure C++) |
| `source/dsp/Echo.h` | Fixed-time feedback echo (pure C++) |
| `source/PluginProcessor.*` | JUCE processor: parameters + the full effect chain |
| `source/PluginEditor.*` | Single-window UI: correction display, power/dual buttons, dropdowns, dials |
| `source/ui/*.h` | LookAndFeel (flat dark theme) + live correction display component |
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
