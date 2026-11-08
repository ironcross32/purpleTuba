// Purple Tuba - the list of current MIDI mappings.
#include "ui/MidiMappingsList.h"

#include "Parameters.h"
#include "ui/Theme.h"

namespace purpletuba
{

MidiMappingsList::MidiMappingsList (MidiLearn& learn)
    : juce::ListBox ("MIDI mappings"), midiLearn (learn)
{
    setModel (this);
    setRowHeight (28);
    setOutlineThickness (1);
    setWantsKeyboardFocus (true);
    refresh();
}

juce::String MidiMappingsList::describe (const MidiLearn::Mapping& m)
{
    return juce::String (paramInfo (m.param).name) + ": CC " + juce::String (m.controller)
         + ", channel " + juce::String (m.channel);
}

void MidiMappingsList::refresh()
{
    mappings = midiLearn.getAllMappings();

    const auto count = (int) mappings.size();
    setTitle (count == 0 ? juce::String ("MIDI mappings, none")
                         : "MIDI mappings, " + juce::String (count));
    setDescription ("Delete removes the selected mapping. Return moves to its control.");

    updateContent();
    if (getSelectedRow() >= count)
        selectRow (count - 1);
    repaint();
}

juce::String MidiMappingsList::getNameForRow (int row)
{
    if (juce::isPositiveAndBelow (row, (int) mappings.size()))
        return describe (mappings[(size_t) row]);
    return {};
}

void MidiMappingsList::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool isSelected)
{
    if (! juce::isPositiveAndBelow (row, (int) mappings.size()))
        return;

    if (isSelected)
        g.fillAll (theme::accent.withAlpha (hasKeyboardFocus (true) ? 0.9f : 0.5f));

    g.setColour (isSelected ? theme::background : theme::text);
    g.setFont (juce::Font (juce::FontOptions (theme::bodyFontSize)));
    g.drawText (describe (mappings[(size_t) row]), 8, 0, width - 16, height, juce::Justification::centredLeft, true);
}

void MidiMappingsList::deleteKeyPressed (int rowIndex)
{
    if (! juce::isPositiveAndBelow (rowIndex, (int) mappings.size()))
        return;

    const auto removed = mappings[(size_t) rowIndex];
    midiLearn.clearMappingForParam (removed.param);
    refresh();

    if (! mappings.empty())
        selectRow (juce::jmin (rowIndex, (int) mappings.size() - 1));

    if (onMappingRemoved)
        onMappingRemoved (removed);
}

void MidiMappingsList::returnKeyPressed (int rowIndex)
{
    if (juce::isPositiveAndBelow (rowIndex, (int) mappings.size()) && onGoToControl)
        onGoToControl (mappings[(size_t) rowIndex].param);
}

} // namespace purpletuba
