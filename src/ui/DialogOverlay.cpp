// Purple Tuba - an accessible in-window dialog.
#include "ui/DialogOverlay.h"

#include "ui/Theme.h"

namespace purpletuba
{

DialogOverlay::DialogOverlay (Options o)
    : options (std::move (o))
{
    setTitle (options.title);
    setDescription (options.longMessage ? juce::String() : options.message);
    setFocusContainerType (juce::Component::FocusContainerType::keyboardFocusContainer);
    setWantsKeyboardFocus (false);

    if (options.longMessage)
    {
        messageText.setMultiLine (true, true);
        messageText.setReadOnly (true);
        messageText.setCaretVisible (true);
        messageText.setScrollbarsShown (true);
        messageText.setFont (juce::Font (juce::FontOptions (theme::bodyFontSize)));
        messageText.setText (options.message, false);
        messageText.setTitle (options.title);
        messageText.setExplicitFocusOrder (1);
        messageText.onEscapeKey = [this] { close (false); };
        messageText.onReturnKey = [this] { close (true); };
        fixTextEditorAccessibility (messageText);
        addAndMakeVisible (messageText);
    }
    else
    {
        messageLabel.setText (options.message, juce::dontSendNotification);
        messageLabel.setJustificationType (juce::Justification::topLeft);
        messageLabel.setMinimumHorizontalScale (1.0f);
        addAndMakeVisible (messageLabel);
    }

    if (options.textInput)
    {
        input.setTitle (options.message);
        input.setFont (juce::Font (juce::FontOptions (theme::bodyFontSize)));
        input.setText (options.initialText, false);
        input.setSelectAllWhenFocused (true);
        input.setExplicitFocusOrder (1);
        input.onReturnKey = [this] { close (true); };
        input.onEscapeKey = [this] { close (false); };
        fixTextEditorAccessibility (input);
        addAndMakeVisible (input);
    }

    okButton.setButtonText (options.okText);
    okButton.setExplicitFocusOrder (2);
    okButton.onClick = [this] { close (true); };
    addAndMakeVisible (okButton);

    if (options.cancelText.isNotEmpty())
    {
        cancelButton.setButtonText (options.cancelText);
        cancelButton.setExplicitFocusOrder (3);
        cancelButton.onClick = [this] { close (false); };
        addAndMakeVisible (cancelButton);
    }
}

std::unique_ptr<juce::AccessibilityHandler> DialogOverlay::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler> (*this, juce::AccessibilityRole::dialogWindow);
}

void DialogOverlay::fixTextEditorAccessibility (juce::TextEditor& editor)
{
   #if JUCE_MAC
    for (auto* child : editor.getChildren())
        child->setAccessible (false);
   #else
    juce::ignoreUnused (editor);
   #endif
}

void DialogOverlay::focusFirst()
{
    if (options.textInput)
        input.grabKeyboardFocus();
    else if (options.longMessage)
        messageText.grabKeyboardFocus();
    else
        okButton.grabKeyboardFocus();
}

void DialogOverlay::close (bool accepted)
{
    if (closing)
        return;
    closing = true;

    // The callback normally deletes this overlay, so copy what it needs first.
    auto callback = options.onClose;
    const auto text = input.getText();
    if (callback)
        callback (accepted, text);
}

bool DialogOverlay::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        close (false);
        return true;
    }
    if (key == juce::KeyPress::returnKey && ! options.textInput)
    {
        close (true);
        return true;
    }
    return false;
}

juce::Rectangle<int> DialogOverlay::panelBounds() const
{
    const int w = juce::jmin (getWidth() - 40, options.longMessage ? 760 : 520);
    const int h = juce::jmin (getHeight() - 40, options.longMessage ? 480 : 220);
    return getLocalBounds().withSizeKeepingCentre (w, h);
}

void DialogOverlay::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.6f));

    const auto panel = panelBounds().toFloat();
    g.setColour (theme::panel);
    g.fillRoundedRectangle (panel, 10.0f);
    g.setColour (theme::accent);
    g.drawRoundedRectangle (panel, 10.0f, 2.0f);

    g.setFont (juce::Font (juce::FontOptions (theme::headingFontSize, juce::Font::bold)));
    g.drawText (options.title, panel.toNearestInt().reduced (16, 10).removeFromTop (28), juce::Justification::centredLeft);
}

void DialogOverlay::resized()
{
    auto area = panelBounds().reduced (16, 10);
    area.removeFromTop (34);

    auto buttons = area.removeFromBottom (36);
    if (options.cancelText.isNotEmpty())
        cancelButton.setBounds (buttons.removeFromRight (110));
    buttons.removeFromRight (10);
    okButton.setBounds (buttons.removeFromRight (110));
    area.removeFromBottom (10);

    if (options.textInput)
    {
        input.setBounds (area.removeFromBottom (34));
        area.removeFromBottom (8);
    }

    if (options.longMessage)
        messageText.setBounds (area);
    else
        messageLabel.setBounds (area);
}

} // namespace purpletuba
