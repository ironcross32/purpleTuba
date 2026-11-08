// Purple Tuba - the editor.
//
// Controls are laid out in titled, screen-reader-visible groups in three columns. Tab order and
// screen reader order follow the groups (Voice source, Tongue, Constriction 1, Constriction 2,
// Nasal, MIDI and output, MIDI learn), column by column, top to bottom.
//
// Alt+L (Command+L on macOS) starts MIDI learn for the focused control, or cancels it; the result
// is announced to the screen reader.
#pragma once

#include "PluginProcessor.h"
#include "ui/Announcer.h"
#include "ui/ControlGroup.h"
#include "ui/DialogOverlay.h"
#include "ui/LearnableControls.h"
#include "ui/MidiMappingsList.h"
#include "ui/Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <memory>
#include <vector>

namespace purpletuba
{

class PurpleTubaEditor final : public juce::AudioProcessorEditor,
                               public ControlHost,
                               private juce::Timer
{
public:
    explicit PurpleTubaEditor (PurpleTubaProcessor& p);
    ~PurpleTubaEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress& key) override;
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

    // ControlHost
    void showControlMenu (LearnableControl& control) override;
    void promptForValue (LearnableControl& control) override;

    // Starts MIDI learn for the focused control, or cancels learn if it is armed.
    void toggleMidiLearn();

    LearnableControl* getControl (int paramIndex) const;
    std::vector<ControlGroup*> getGroupsInOrder() const;

private:
    void timerCallback() override;

    void buildParameterGroups();
    void buildLearnGroup();

    void startLearn (LearnableControl& control);
    void cancelLearn();
    void syncLearnState();
    void refreshMappings();

    void moveToGroup (int delta);
    void focusFirstControlIfNothingFocused();

    void showDialog (DialogOverlay::Options options);
    void closeDialog();
    void showShortcuts();
    void setParameterFromText (int paramIndex, const juce::String& text);

    PurpleTubaProcessor& tuba;
    theme::LookAndFeel lookAndFeel;
    juce::TooltipWindow tooltipWindow { this, 800 };

    // Holds the groups. It is only a layout container, so screen readers skip it and see the
    // groups directly.
    struct Content final : juce::Component
    {
        std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
        {
            return std::make_unique<juce::AccessibilityHandler> (*this, juce::AccessibilityRole::ignored);
        }
    };
    Content content;
    juce::Label titleLabel, hintLabel;

    std::array<std::unique_ptr<ControlGroup>, numGroups> paramGroups;
    std::unique_ptr<ControlGroup> learnGroup;
    std::array<std::unique_ptr<LearnableControl>, numParams> controls;

    juce::Label learnStatus;
    MidiMappingsList mappingsList;
    juce::TextButton clearAllButton { "Clear all MIDI mappings" };
    juce::TextButton shortcutsButton { "Keyboard shortcuts" };

    std::unique_ptr<DialogOverlay> dialog, retiredDialog;
    juce::Component::SafePointer<juce::Component> focusBeforeDialog;

    a11y::AnnouncementQueue announcements;
    uint32_t lastMappingVersion = 0;
    int lastArmedParam = -1;
    juce::String lastLearnedText;
    bool sentStructureChange = false;
    bool initialFocusDone = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PurpleTubaEditor)
};

} // namespace purpletuba
