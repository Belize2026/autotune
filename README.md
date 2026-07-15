# AUTISMIDOL AUTOTUNE

A VST3/AU plugin that does one thing well: **instant, hard pitch snapping with
no glide**. This is the extreme robotic autotune sound used in hyperpop and
melodic rap — not a natural-sounding corrector.

(The plugin is named **AUTISMIDOL AUTOTUNE** everywhere you'll see it — in
your DAW, the installers, and the plugin window. `HardTune` survives only as
the internal code name in this repo's source.)

## Download & install

Grab the installer for your OS from the
**[latest release](https://github.com/Belize2026/autotune/releases/latest)**
(no GitHub account needed):

- **macOS**: [`AUTISMIDOL-AUTOTUNE-Installer-macOS.pkg`](https://github.com/Belize2026/autotune/releases/latest/download/AUTISMIDOL-AUTOTUNE-Installer-macOS.pkg)
- **Windows**: [`AUTISMIDOL-AUTOTUNE-Installer-Windows.exe`](https://github.com/Belize2026/autotune/releases/latest/download/AUTISMIDOL-AUTOTUNE-Installer-Windows.exe)

### macOS (Logic Pro, Ableton, etc.)

1. Download the `.pkg` above.
2. **Right-click it → Open** (double-clicking gets blocked because the
   installer is unsigned; right-click → Open shows an "Open" button instead).
   If macOS still refuses: System Settings → Privacy & Security → scroll down
   → **Open Anyway**.
3. Follow the installer (it asks for your password). It installs:
   - `AUTISMIDOL AUTOTUNE.component` (AU, what Logic uses) → `/Library/Audio/Plug-Ins/Components`
   - `AUTISMIDOL AUTOTUNE.vst3` → `/Library/Audio/Plug-Ins/VST3`
4. Open your DAW. In Logic the plugin appears on a channel strip under
   **Audio FX → Audio Units → AutismIdol → AUTISMIDOL AUTOTUNE**. If it
   doesn't show up: Logic Pro → Settings → Plug-in Manager → **Reset &
   Rescan Selection**, then restart Logic.

### Windows

1. Download the `.exe` above.
2. If SmartScreen pops up, click **More info → Run anyway** (the installer is
   unsigned).
3. Follow the installer. It installs `AUTISMIDOL AUTOTUNE.vst3` into
   `C:\Program Files\Common Files\VST3` — the standard folder every VST3 DAW
   scans.
4. Rescan plugins in your DAW (or just restart it). AUTISMIDOL AUTOTUNE
   appears under AutismIdol.

### Uninstalling

- **macOS**: delete `AUTISMIDOL AUTOTUNE.component` and
  `AUTISMIDOL AUTOTUNE.vst3` from the two folders listed above.
- **Windows**: Settings → Apps → Installed apps → AUTISMIDOL AUTOTUNE →
  Uninstall.

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
- YIN pitch detection over a tiny (~15 ms) window, re-checked every ~3 ms,
  with an aggressive grab-anyway fallback on marginal frames — the tuner
  clamps on instantly and never relaxes mid-phrase. A 2-detection (~6 ms)
  note confirmation keeps note changes as clean audible steps instead of
  chatter. Harsh and artificial on purpose; that is the selling point
- Hard quantisation to the nearest note of the selected key/scale: zero
  tolerance, zero glide, no partial correction
- Time-domain pitch shifting (dual-tap crossfaded delay line), no formant
  preservation
- Controls: On/Off, Key (C–B), Scale (Chromatic / Major / Minor / Minor
  Pentatonic), Cronk dial, Formant dial, Dual Vocals selector, Echo dial,
  Reverb dial
- **Cronk** (0–100 %, clock-face dial): the harshness control. It scales
  the shifter's sweep window log-spaced from 50 ms (0 %, smooth) down to
  14 ms (100 %, maximum grit) — the floor is set where the sound stays
  clean enough for professional use. Corrections are instant at every
  setting — cronk shapes texture, not speed. Ratio changes ride a ~5 ms
  de-click ramp, so note jumps are clean but still sound instant.
- Formant shift: PSOLA-style pitch-synchronous granular processing driven by
  the tuner's pitch tracking — reshapes the main vocal's character (±12 st)
  while the tuned pitch stays put.
- **Dual Vocals** harmony selector (Off / 3rd Up / 5th Up / Octave Up /
  Octave Down) with a **Dual Level** dial: Off by default. The main vocal
  always stays on level; the harmony voice is layered underneath at the
  chosen level. 3rd and 5th follow the selected key/scale like a real
  harmonizer (fixed major-3rd/perfect-5th in Chromatic).
- Echo: a single rotary dial (0–100 %), fixed 375 ms feedback delay that
  self-ducks: repeats pull back while the vocal is present and bloom into
  the gaps, so the mix never gets muddy. At 0 it is fully inactive.
- Reverb: same dial-as-activator pattern (0–100 %) after the echo. Both work
  independently of the tune On/Off button, so the dry bypass can still carry
  space.
- Pitch-correction meter (Antares-style): a green → red scale showing how
  hard the tuner is pulling — green means the note barely needed
  correcting, red means the original note was way off — with the current
  target note named large beside it.

Signal chain: detect → quantise → shift → formant → dual voice → echo →
reverb.

Explicitly **not** in v1: speed knob, wet/dry mix for the tuner itself.

## Manual install (no installer)

Each release also carries the raw plugin bundles as zips
(`AUTISMIDOL-AUTOTUNE-AU-macOS.zip`, `AUTISMIDOL-AUTOTUNE-VST3-macOS.zip`,
`AUTISMIDOL-AUTOTUNE-VST3-Windows.zip`, `AUTISMIDOL-AUTOTUNE-VST3-Linux.zip`)
if you'd rather place the files yourself.

On a Mac, unzip and drop:

- `AUTISMIDOL AUTOTUNE.component` → `~/Library/Audio/Plug-Ins/Components/`
- `AUTISMIDOL AUTOTUNE.vst3` → `~/Library/Audio/Plug-Ins/VST3/`

Manually downloaded bundles get quarantined by macOS (they are ad-hoc
signed, not notarised — another reason the `.pkg` installer is the easier
route, since installed files skip quarantine). After copying them into
place, clear the flag once:

```sh
xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/Components/AUTISMIDOL AUTOTUNE.component"
xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/AUTISMIDOL AUTOTUNE.vst3"
```

Then restart Logic (Settings → Plug-in Manager → Reset & Rescan if needed).

On Windows, unzip and drop `AUTISMIDOL AUTOTUNE.vst3` into
`C:\Program Files\Common Files\VST3`.

## Dev builds (every push)

Every push also builds everything via GitHub Actions: repo → **Actions** →
latest **Build plugins** run → **Artifacts** (requires being signed in to
GitHub; artifacts expire after 90 days). That includes the installers, the
platform zips, and the raw Windows `AUTISMIDOL-AUTOTUNE.vst3` bundle as its
own artifact.

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

- Detection window: `sr * 0.015` in `PitchDetector::prepare` — shorter is
  twitchier and more robotic, longer is steadier.
- Sweep window range: `minWindowSeconds` / `maxWindowSeconds` in
  `PitchShifter` — what the CRONK dial maps onto.
- Note confirmation: the 2-detection rule in `PluginProcessor` — raise it
  for steadier notes, drop to 1 for maximum chatter.
- Grab aggression: `fallbackThreshold` in `PitchDetector` — higher grabs even
  noisier frames.
