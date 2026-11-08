// Purple Tuba - keyboard shortcuts.
#include "ui/KeyboardShortcuts.h"

namespace purpletuba::shortcuts
{

namespace
{
    constexpr int menuKeyCode = 93; // VK_APPS, the Windows "Applications"/Menu key (as Surge XT uses)

    // Compares a key press to a letter/punctuation key + exact modifier set, ignoring case and
    // mouse buttons.
    bool matches (const juce::KeyPress& key, int keyCode, juce::ModifierKeys mods) noexcept
    {
        const auto pressed = key.getModifiers();
        constexpr int keyFlags = juce::ModifierKeys::shiftModifier | juce::ModifierKeys::ctrlModifier
                               | juce::ModifierKeys::altModifier | juce::ModifierKeys::commandModifier;

        if ((pressed.getRawFlags() & keyFlags) != (mods.getRawFlags() & keyFlags))
            return false;

        const auto a = juce::CharacterFunctions::toLowerCase ((juce::juce_wchar) key.getKeyCode());
        const auto b = juce::CharacterFunctions::toLowerCase ((juce::juce_wchar) keyCode);
        return a == b;
    }
}

juce::ModifierKeys midiLearnModifier() noexcept
{
   #if JUCE_MAC
    return juce::ModifierKeys (juce::ModifierKeys::commandModifier);
   #else
    return juce::ModifierKeys (juce::ModifierKeys::altModifier);
   #endif
}

juce::ModifierKeys groupModifier() noexcept
{
    return juce::ModifierKeys (juce::ModifierKeys::altModifier);
}

juce::KeyPress midiLearnKey()     { return juce::KeyPress ('l', midiLearnModifier(), 0); }
juce::KeyPress nextGroupKey()     { return juce::KeyPress ('.', groupModifier(), 0); }
juce::KeyPress previousGroupKey() { return juce::KeyPress (',', groupModifier(), 0); }

bool isMidiLearn (const juce::KeyPress& key) noexcept     { return matches (key, 'l', midiLearnModifier()); }
bool isNextGroup (const juce::KeyPress& key) noexcept     { return matches (key, '.', groupModifier()); }
bool isPreviousGroup (const juce::KeyPress& key) noexcept { return matches (key, ',', groupModifier()); }

bool isHelp (const juce::KeyPress& key) noexcept
{
    return key.getKeyCode() == juce::KeyPress::F1Key && ! key.getModifiers().isAnyModifierKeyDown();
}

bool isContextMenu (const juce::KeyPress& key) noexcept
{
    const auto mods = key.getModifiers();
    if (key.getKeyCode() == juce::KeyPress::F10Key && mods.isShiftDown() && ! mods.isCtrlDown()
        && ! mods.isAltDown() && ! mods.isCommandDown())
        return true;
    return key.getKeyCode() == menuKeyCode && ! mods.isAnyModifierKeyDown();
}

juce::String describe (const juce::KeyPress& key)
{
    juce::StringArray parts;
    const auto mods = key.getModifiers();

   #if JUCE_MAC
    if (mods.isCtrlDown())    parts.add ("Control");
    if (mods.isAltDown())     parts.add ("Option");
    if (mods.isShiftDown())   parts.add ("Shift");
    if (mods.isCommandDown()) parts.add ("Command");
   #else
    if (mods.isCtrlDown())    parts.add ("Ctrl");
    if (mods.isAltDown())     parts.add ("Alt");
    if (mods.isShiftDown())   parts.add ("Shift");
   #endif

    const auto code = key.getKeyCode();
    if (code == '.')      parts.add ("Period");
    else if (code == ',') parts.add ("Comma");
    else if (code > 32 && code < 127) parts.add (juce::String::charToString ((juce::juce_wchar) code).toUpperCase());
    else parts.add (juce::KeyPress (code).getTextDescription());

    return parts.joinIntoString ("+");
}

juce::String midiLearnText() { return describe (midiLearnKey()); }

juce::String helpText()
{
    const auto learn = midiLearnText();
    juce::StringArray lines;
    lines.add (learn + ": start MIDI learn for the focused control, or cancel it. Then move a knob, fader or pedal on your MIDI controller.");
    lines.add (describe (nextGroupKey()) + ": move to the next group of controls.");
    lines.add (describe (previousGroupKey()) + ": move to the previous group of controls.");
    lines.add ("Tab and Shift+Tab: move between controls.");
    lines.add ("Up or Right arrow: increase. Down or Left arrow: decrease. Hold Shift for fine steps.");
    lines.add ("Page Up and Page Down: large steps.");
    lines.add ("Home: maximum. End: minimum. Delete or Backspace: reset to default.");
    lines.add ("Return: type a value. Space: switch a toggle on or off.");
   #if JUCE_MAC
    lines.add ("Shift+F10, or Control-click: control menu with MIDI learn, clear mapping, enter value and reset.");
   #else
    lines.add ("Shift+F10 or the Applications key: control menu with MIDI learn, clear mapping, enter value and reset.");
   #endif
    lines.add ("Delete in the MIDI mappings list: remove the selected mapping. Return: go to that control.");
    lines.add ("F1: show this list.");
    return lines.joinIntoString ("\n");
}

} // namespace purpletuba::shortcuts
