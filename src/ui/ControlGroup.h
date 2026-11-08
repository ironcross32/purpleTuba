// Purple Tuba - a titled group of controls.
//
// Exposed to screen readers as a named group (AccessibilityRole::group) and made a focus
// container, so VoiceOver can interact into it and NVDA / JAWS / Narrator announce its name when
// focus enters it. It is not a *keyboard* focus container, so Tab still moves on to the next group.
// Modelled on Surge XT's OverlayAsAccessibleContainer.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace purpletuba
{

class ControlGroup final : public juce::Component
{
public:
    ControlGroup (const juce::String& title, const juce::String& description, int groupOrder);

    // A labelled row: [name] [control] [value]. The labels are visual only (hidden from screen
    // readers) because the control itself carries the name and the value.
    juce::Label& addRow (juce::Component& control, const juce::String& name, bool withValueLabel);

    // A control that spans the full width of the group, with a given height.
    void addFullWidth (juce::Component& control, int height);

    juce::Label* getValueLabel (juce::Component& control);

    // The first focusable control, for group navigation.
    juce::Component* getFirstControl() const;
    const std::vector<juce::Component*>& getControls() const noexcept { return controlsInOrder; }

    int getPreferredHeight() const;

    void paint (juce::Graphics& g) override;
    void resized() override;

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

    static constexpr int headerHeight = 30;
    static constexpr int rowHeight = 34;
    static constexpr int padding = 8;

private:
    struct Row
    {
        juce::Component* control = nullptr;
        std::unique_ptr<juce::Label> nameLabel;
        std::unique_ptr<juce::Label> valueLabel;
        int height = rowHeight;
        bool fullWidth = false;
    };

    std::vector<Row> rows;
    std::vector<juce::Component*> controlsInOrder;
};

} // namespace purpletuba
