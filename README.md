# Purple Tuba

A vocal tract synthesizer plug-in (VST3, CLAP and Standalone). It is built with JUCE and
based on Neil Thapen's [Pink Trombone](https://dood.al/pinktrombone/).

Pink Trombone is played by dragging several fingers across a drawing of a mouth. Purple Tuba
replaces the drawing with grouped, labelled controls that work with a **screen reader** and a
keyboard. **Every control that shapes the voice can be MIDI-learned**, so a hardware
controller can play the tongue, lips and voice at the same time, the way fingers do in the
original.

## Building

You need CMake 3.22 or later, a C++20 compiler (Visual Studio 2022/2026, Xcode or GCC/Clang),
and the git submodules:

```
git clone --recurse-submodules <repo-url>
# or, in an existing clone:
git submodule update --init --recursive
```

Windows (PowerShell):

```powershell
cmake -G "Visual Studio 18 2026" -A x64 -S . -B build
cmake --build build --config Release --target PurpleTuba_VST3 PurpleTuba_CLAP PurpleTuba_Standalone
ctest --test-dir build -C Release --output-on-failure
```

macOS / Linux:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target PurpleTuba_VST3 PurpleTuba_CLAP PurpleTuba_Standalone
ctest --test-dir build -C Release --output-on-failure
```

Build output goes to `build/PurpleTuba_artefacts/<Config>/{VST3,CLAP,Standalone}/`. Pass
`-DPURPLETUBA_COPY_AFTER_BUILD=ON` to have the build copy the plug-ins into the system plug-in
folders. On Windows that needs write access to `C:\Program Files\Common Files\VST3`.

## Playing it

- **MIDI notes** set the pitch and start the voice. It is mono with last-note priority, and
  supports glide and pitch bend.
- **Always voice** keeps it sounding without notes, using the **Pitch** control, like the
  original's "always voice" button.
- **Tongue position** and **Tongue height** choose the vowel: back and low gives "ah", front
  and high gives "ee".
- **Constriction 1** and **Constriction 2** are the original's extra fingers. Set each one's
  position along the tract, then raise **Closure**. Narrow settings hiss (s, sh, f), 100 %
  closes the tract, and quickly reopening it gives a plosive (t, p, k). By default,
  Constriction 1 sits at the tongue tip and Constriction 2 at the lips.
- **Nasal opening** lowers the velum. Close the lips and open the nose to hum "m".

## Accessibility

- The controls are in seven named groups: Voice source, Tongue, Constriction 1,
  Constriction 2, Nasal, MIDI and output, and MIDI learn. Screen readers announce the group
  name when focus enters it.
- With VoiceOver you can interact with each group (VO+Shift+Down Arrow).
- Tab order, screen reader order and visual order all match: the groups run column by column,
  top to bottom.
- Every control has an accessible name and a spoken value, such as "40 %, front",
  "140 Hz, C#3" or "85 %, fricative". Its description holds help text and its current MIDI
  mapping.
- On Windows, announcements are UI Automation notification events, so NVDA, JAWS and Narrator
  speak them in your own screen reader voice, not through SAPI. On macOS they are VoiceOver
  announcements.
- Dialogs (value entry and the shortcut list) open inside the plug-in window, keep Tab
  inside themselves, and close with Escape.

### Keyboard

| Keys | Action |
|---|---|
| **Alt+L** (Windows/Linux), **Command+L** (macOS) | Start MIDI learn for the focused control, or cancel it |
| Alt+Period / Alt+Comma (Option on macOS) | Next / previous group |
| Up/Right, Down/Left | Increase / decrease (hold Shift for fine steps) |
| Page Up / Page Down | Large steps |
| Home / End | Maximum / minimum |
| Delete or Backspace | Reset to default |
| Return | Type a value |
| Shift+F10 or the Applications key | Control menu: MIDI learn, clear mapping, enter value, reset |
| Escape | Cancel MIDI learn, or close a dialog |
| F1 | List the keyboard shortcuts |

### MIDI learn

1. Move focus to a control, for example **Tongue position**.
2. Press **Alt+L** (**Command+L** on a Mac). You will hear "MIDI learn for Tongue position.
   Move a control on your MIDI device."
3. Move a knob, fader or pedal. You will hear "Tongue position learned: CC 74, channel 1."
   Pressing Alt+L again before you move anything cancels learn.

A controller drives one control at a time, and a control listens to one controller. Learning
again moves the mapping. Mappings are saved with your project. The **MIDI mappings** list in
the MIDI learn group shows them all: press Delete to remove the selected one, or Return to jump
to its control. Bank select (CC 0/32) and channel-mode messages (CC 120–127) are never learned.

In VST3 hosts, CCs arrive through JUCE's MIDI-CC parameter emulation, so no host setup is
needed. Some hosts only send keys to a plug-in window that has keyboard focus, so click into
the window, or use the host's own "send all keystrokes to plug-in" option if it has one.

## How it works

`src/dsp/` is a direct port of Pink Trombone's DSP:

- A Liljencrants-Fant glottal source with vibrato and simplex-noise wobble.
- A 44-section Kelly-Lochbaum waveguide tract with a 28-section nasal branch, reflection
  interpolation, closure transients and turbulence noise injected at constrictions.
- Aspiration and fricative noise from band-passed white noise.

The tract's formants depend on its step rate, so the engine always runs at **48 kHz**. The
processor resamples to the host rate with a windowed-sinc interpolator, and the unit tests
check that the voice sounds the same at 44.1, 48 and 96 kHz.

## License

AGPL-3.0 (see `LICENSE`). Pink Trombone is © 2017 Neil Thapen under the MIT license. Its
notice, and the other third-party notices, are in `THIRD_PARTY_NOTICES.md`. JUCE is used under
the AGPLv3. Distributing closed-source builds needs a commercial JUCE licence.
