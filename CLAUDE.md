# CLAUDE.md

Guidance for working in this repository.

## Project

Purple Tuba is a JUCE 9 vocal-tract synth plug-in: VST3, CLAP (via clap-juce-extensions) and
Standalone. It ports Pink Trombone's DSP (MIT, Neil Thapen) and replaces its multi-touch canvas
with grouped, screen-reader-accessible, MIDI-learnable controls. Submodules live in `libs/`
(JUCE pinned to 9.0.5, clap-juce-extensions, Catch2). License: AGPL-3.0.

## Build & test (Windows, PowerShell)

```powershell
cmake -G "Visual Studio 18 2026" -A x64 -S . -B build
cmake --build build --config Debug --target PurpleTubaTests
ctest --test-dir build -C Debug --output-on-failure
cmake --build build --config Release --target PurpleTuba_VST3 PurpleTuba_CLAP PurpleTuba_Standalone
```

Sources are collected with `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`. The tests link the
plug-in's shared-code static library and borrow its include paths and definitions
(`tests/CMakeLists.txt`).

## Architecture

- `src/Parameters.{h,cpp}`: **the single parameter table**. ID, accessible name, group, range,
  default, value-text format and help text all live here. The APVTS layout, editor controls,
  accessible names and descriptions, and MIDI learn are all generated from it. To add a
  parameter, add a `ParamId`, a table row in the same order, and read it in
  `PurpleTubaProcessor::updateEngineParams()`. The editor picks it up automatically.
- `src/dsp/`: the Pink Trombone port. `Glottis`, `Tract` (with nose, transients and
  turbulence), `Noise` (simplex, white noise, Web Audio-style bandpass) and `VoiceEngine`, which
  maps the 0..1 user controls onto the original's touch units and renders 64-sample control
  blocks. **The engine always runs at 48 kHz**. `PurpleTubaProcessor::renderHost` resamples with
  `juce::WindowedSincInterpolator`. Keep the original's constants. Rates that were "per
  512-sample block" in the original are rescaled to per-second rates.
- `src/midi/MidiLearn`: lock-free CC→parameter table (`slotToParam`/`paramToSlot` atomics). It
  is armed from the editor and resolved on the audio thread in `handleController`, and learn
  events go to the editor through an SPSC ring. Mappings are saved in plugin state as a
  `MidiMappings` child of the APVTS tree. `getStateInformation` strips any stale copy before
  appending.
- `src/ui/`:
  - `PluginEditor`: the layout, the Alt/Cmd+L learn flow, the context menu, value entry, group
    navigation and a 30 Hz timer that drains learn events and announcements.
  - `ControlGroup`: titled `AccessibilityRole::group`, set as a `focusContainer` (not a
    keyboard container, so Tab leaves the group).
  - `LearnableControls`: Slider and ToggleButton subclasses with Surge-style key handling.
  - `Announcer`: UIA `UiaRaiseNotificationEvent` on Windows, JUCE `postAnnouncement` elsewhere.
    Announcements are queued for a few ticks so the focus change that triggered them is
    spoken first.
  - `DialogOverlay`: an in-window accessible dialog.
  - `MidiMappingsList`.
  - `KeyboardShortcuts`: platform keys. Learn is Alt+L, or Cmd+L on macOS.

## Accessibility rules for changes

- Every new control needs a title. Put visible-only labels at `setAccessible(false)` so they are
  not read twice. Values must read well as text, through `formatValue`.
- Keep group order = Tab order = visual reading order: explicit focus orders, columns top to
  bottom.
- Never call `AccessibilityHandler::postAnnouncement` directly on Windows (it uses SAPI). Use
  `a11y::announce` or the editor's `announcements` queue.
- After a popup menu closes, focus is restored asynchronously with give-away and re-grab
  (see `showControlMenu`). Otherwise UIA reports the window as focused.
- `C:\programming\surge` is a read-only reference for accessibility patterns. Don't copy code
  from it.
