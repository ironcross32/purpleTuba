// Purple Tuba - a titled group of controls.
#include "ui/ControlGroup.h"

#include "ui/Theme.h"

namespace purpletuba
{

ControlGroup::ControlGroup (const juce::String& title, const juce::String& description, int groupOrder)
{
    setName (title);
    setTitle (title);
    setDescription (description);
    setAccessible (true);
    setFocusContainerType (juce::Component::FocusContainerType::focusContainer);
    setExplicitFocusOrder (groupOrder);
    setWantsKeyboardFocus (false);
}

std::unique_ptr<juce::AccessibilityHandler> ControlGroup::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler> (*this, juce::AccessibilityRole::group);
}

juce::Label& ControlGroup::addRow (juce::Component& control, const juce::String& name, bool withValueLabel)
{
    Row row;
    row.control = &control;

    row.nameLabel = std::make_unique<juce::Label> (juce::String(), name);
    row.nameLabel->setAccessible (false);
    row.nameLabel->setJustificationType (juce::Justification::centredLeft);
    row.nameLabel->setInterceptsMouseClicks (false, false);
    addAndMakeVisible (*row.nameLabel);

    if (withValueLabel)
    {
        row.valueLabel = std::make_unique<juce::Label>();
        row.valueLabel->setAccessible (false);
        row.valueLabel->setJustificationType (juce::Justification::centredRight);
        row.valueLabel->setColour (juce::Label::textColourId, theme::dimText);
        row.valueLabel->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*row.valueLabel);
    }

    control.setExplicitFocusOrder ((int) controlsInOrder.size() + 1);
    addAndMakeVisible (control);
    controlsInOrder.push_back (&control);

    rows.push_back (std::move (row));
    resized();
    return *rows.back().nameLabel;
}

void ControlGroup::addFullWidth (juce::Component& control, int height)
{
    Row row;
    row.control = &control;
    row.height = height;
    row.fullWidth = true;

    control.setExplicitFocusOrder ((int) controlsInOrder.size() + 1);
    addAndMakeVisible (control);
    controlsInOrder.push_back (&control);

    rows.push_back (std::move (row));
    resized();
}

juce::Label* ControlGroup::getValueLabel (juce::Component& control)
{
    for (auto& r : rows)
        if (r.control == &control)
            return r.valueLabel.get();
    return nullptr;
}

juce::Component* ControlGroup::getFirstControl() const
{
    for (auto* c : controlsInOrder)
        if (c->isVisible() && c->getWantsKeyboardFocus())
            return c;
    return nullptr;
}

int ControlGroup::getPreferredHeight() const
{
    int h = headerHeight + padding;
    for (const auto& r : rows)
        h += r.height;
    return h + padding;
}

void ControlGroup::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (theme::panel);
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (theme::panelBorder);
    g.drawRoundedRectangle (bounds, 8.0f, 1.5f);

    g.setColour (theme::accent);
    g.setFont (juce::Font (juce::FontOptions (theme::headingFontSize, juce::Font::bold)));
    g.drawText (getTitle(), getLocalBounds().removeFromTop (headerHeight).reduced (padding + 4, 0),
                juce::Justification::centredLeft, true);
}

void ControlGroup::resized()
{
    auto area = getLocalBounds().reduced (padding);
    area.removeFromTop (headerHeight - padding);

    const int nameWidth = juce::jlimit (110, 220, area.getWidth() * 36 / 100);
    const int valueWidth = juce::jlimit (90, 160, area.getWidth() * 30 / 100);

    for (auto& r : rows)
    {
        auto rowArea = area.removeFromTop (r.height);

        if (r.fullWidth)
        {
            r.control->setBounds (rowArea.reduced (2));
            continue;
        }

        if (r.nameLabel != nullptr)
            r.nameLabel->setBounds (rowArea.removeFromLeft (nameWidth));
        if (r.valueLabel != nullptr)
            r.valueLabel->setBounds (rowArea.removeFromRight (valueWidth));
        r.control->setBounds (rowArea.reduced (2));
    }
}

} // namespace purpletuba
