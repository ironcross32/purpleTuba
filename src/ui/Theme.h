// Purple Tuba - colours and look-and-feel. High contrast, large text.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace purpletuba::theme
{

inline const juce::Colour background   { 0xff1b1426 };
inline const juce::Colour panel        { 0xff2a1f3d };
inline const juce::Colour panelBorder  { 0xff6b4f99 };
inline const juce::Colour text         { 0xfff4eefc };
inline const juce::Colour dimText      { 0xffc9bde0 };
inline const juce::Colour accent       { 0xffc58cff };
inline const juce::Colour track        { 0xff4a3a66 };
inline const juce::Colour focusColour  { 0xffffffff };
inline const juce::Colour learnColour  { 0xffffc23d };

constexpr float bodyFontSize = 16.0f;
constexpr float headingFontSize = 18.0f;

class LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    LookAndFeel()
    {
        setColour (juce::ResizableWindow::backgroundColourId, background);
        setColour (juce::Label::textColourId, text);
        setColour (juce::Slider::thumbColourId, accent);
        setColour (juce::Slider::trackColourId, accent.withAlpha (0.8f));
        setColour (juce::Slider::backgroundColourId, track);
        setColour (juce::ToggleButton::textColourId, text);
        setColour (juce::ToggleButton::tickColourId, accent);
        setColour (juce::ToggleButton::tickDisabledColourId, dimText);
        setColour (juce::TextButton::buttonColourId, panelBorder);
        setColour (juce::TextButton::textColourOffId, text);
        setColour (juce::ListBox::backgroundColourId, background);
        setColour (juce::ListBox::outlineColourId, panelBorder);
        setColour (juce::TextEditor::backgroundColourId, background);
        setColour (juce::TextEditor::textColourId, text);
        setColour (juce::TextEditor::outlineColourId, panelBorder);
        setColour (juce::TextEditor::focusedOutlineColourId, focusColour);
        setColour (juce::PopupMenu::backgroundColourId, panel);
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accent);
        setColour (juce::PopupMenu::highlightedTextColourId, background);
        setColour (juce::TooltipWindow::backgroundColourId, panel);
        setColour (juce::TooltipWindow::textColourId, text);
        setColour (juce::TooltipWindow::outlineColourId, panelBorder);
    }

    juce::Font getLabelFont (juce::Label&) override { return juce::Font (juce::FontOptions (bodyFontSize)); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return juce::Font (juce::FontOptions (bodyFontSize)); }
    juce::Font getPopupMenuFont() override { return juce::Font (juce::FontOptions (bodyFontSize)); }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool highlighted, bool down) override
    {
        const auto bounds = button.getLocalBounds().toFloat().reduced (4.0f, 2.0f);
        const float boxSize = juce::jmin (20.0f, bounds.getHeight() - 4.0f);
        const juce::Rectangle<float> box (bounds.getX() + 4.0f, bounds.getCentreY() - boxSize / 2, boxSize, boxSize);

        g.setColour (highlighted || down ? accent.brighter (0.2f) : accent);
        g.drawRoundedRectangle (box, 3.0f, 2.0f);
        if (button.getToggleState())
        {
            g.fillRoundedRectangle (box.reduced (4.0f), 2.0f);
        }

        g.setColour (text);
        g.setFont (juce::Font (juce::FontOptions (bodyFontSize)));
        auto textArea = bounds.withLeft (box.getRight() + 10.0f);
        g.drawFittedText (button.getButtonText() + (button.getToggleState() ? "  (On)" : "  (Off)"),
                          textArea.toNearestInt(), juce::Justification::centredLeft, 1);
    }
};

} // namespace purpletuba::theme
