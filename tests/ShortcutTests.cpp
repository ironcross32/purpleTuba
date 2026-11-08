#include "ui/KeyboardShortcuts.h"

#include <catch2/catch_test_macros.hpp>

using namespace purpletuba;
using juce::KeyPress;
using juce::ModifierKeys;

TEST_CASE ("MIDI learn is Alt+L on Windows/Linux and Command+L on macOS", "[shortcuts]")
{
   #if JUCE_MAC
    const ModifierKeys learnMods (ModifierKeys::commandModifier);
    const ModifierKeys otherMods (ModifierKeys::altModifier);
    CHECK (shortcuts::midiLearnText() == "Command+L");
   #else
    const ModifierKeys learnMods (ModifierKeys::altModifier);
    const ModifierKeys otherMods (ModifierKeys::ctrlModifier);
    CHECK (shortcuts::midiLearnText() == "Alt+L");
   #endif

    CHECK (shortcuts::isMidiLearn (KeyPress ('l', learnMods, 0)));
    CHECK (shortcuts::isMidiLearn (KeyPress ('L', learnMods, 0)));
    CHECK_FALSE (shortcuts::isMidiLearn (KeyPress ('l', otherMods, 0)));
    CHECK_FALSE (shortcuts::isMidiLearn (KeyPress ('l', ModifierKeys(), 0)));
    CHECK_FALSE (shortcuts::isMidiLearn (KeyPress ('l', learnMods.withFlags (ModifierKeys::shiftModifier), 0)));
    CHECK_FALSE (shortcuts::isMidiLearn (KeyPress ('k', learnMods, 0)));
}

TEST_CASE ("Group navigation and menu keys", "[shortcuts]")
{
    const ModifierKeys alt (ModifierKeys::altModifier);
    CHECK (shortcuts::isNextGroup (KeyPress ('.', alt, 0)));
    CHECK (shortcuts::isPreviousGroup (KeyPress (',', alt, 0)));
    CHECK_FALSE (shortcuts::isNextGroup (KeyPress ('.', ModifierKeys(), 0)));

    CHECK (shortcuts::isContextMenu (KeyPress (KeyPress::F10Key, ModifierKeys (ModifierKeys::shiftModifier), 0)));
    CHECK_FALSE (shortcuts::isContextMenu (KeyPress (KeyPress::F10Key, ModifierKeys(), 0)));
    CHECK (shortcuts::isHelp (KeyPress (KeyPress::F1Key)));

    CHECK (shortcuts::helpText().contains (shortcuts::midiLearnText()));
}
