// Purple Tuba - the list of current MIDI mappings.
#pragma once

#include "midi/MidiLearn.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <vector>

namespace purpletuba
{

class MidiMappingsList final : public juce::ListBox, private juce::ListBoxModel
{
public:
    explicit MidiMappingsList (MidiLearn& learn);

    // Re-reads the mappings from MidiLearn.
    void refresh();

    static juce::String describe (const MidiLearn::Mapping& m);

    std::function<void (const MidiLearn::Mapping&)> onMappingRemoved;
    std::function<void (int paramIndex)> onGoToControl;

private:
    int getNumRows() override { return (int) mappings.size(); }
    void paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool isSelected) override;
    juce::String getNameForRow (int row) override;
    void deleteKeyPressed (int rowIndex) override;
    void backgroundClicked (const juce::MouseEvent&) override {}
    void returnKeyPressed (int rowIndex) override;
    void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override { returnKeyPressed (row); }

    MidiLearn& midiLearn;
    std::vector<MidiLearn::Mapping> mappings;
};

} // namespace purpletuba
