// Purple Tuba - keyboard shortcuts.
//
// MIDI learn is Alt+L on Windows and Linux, and Command+L on macOS.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace purpletuba::shortcuts
{

// The modifier for MIDI learn: Command on macOS, Alt elsewhere.
juce::ModifierKeys midiLearnModifier() noexcept;

// The modifier for group navigation (Alt / Option).
juce::ModifierKeys groupModifier() noexcept;

juce::KeyPress midiLearnKey();
juce::KeyPress nextGroupKey();
juce::KeyPress previousGroupKey();

bool isMidiLearn (const juce::KeyPress& key) noexcept;
bool isNextGroup (const juce::KeyPress& key) noexcept;
bool isPreviousGroup (const juce::KeyPress& key) noexcept;
bool isHelp (const juce::KeyPress& key) noexcept;        // F1
bool isContextMenu (const juce::KeyPress& key) noexcept; // Shift+F10 or the Menu (Applications) key

// Human-readable names using the platform's modifier names ("Alt+L", "Command+L").
juce::String describe (const juce::KeyPress& key);
juce::String midiLearnText();

// The full list shown by the Keyboard shortcuts dialog.
juce::String helpText();

} // namespace purpletuba::shortcuts
