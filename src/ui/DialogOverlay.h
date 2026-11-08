// Purple Tuba - an accessible in-window dialog (value entry, keyboard shortcut help).
//
// Plug-in windows and separate desktop dialogs do not mix well across hosts, so this is drawn
// inside the editor. It is a keyboard focus container (Tab stays inside it), has the dialog
// role, and the editor hides everything behind it from screen readers while it is open.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>

namespace purpletuba
{

class DialogOverlay final : public juce::Component
{
public:
    struct Options
    {
        juce::String title;
        juce::String message;
        bool textInput = false;           // show a single-line text field
        juce::String initialText;
        bool longMessage = false;         // show the message in a read-only multi-line text box
        juce::String okText = "OK";
        juce::String cancelText = "Cancel"; // empty: no cancel button
        std::function<void (bool accepted, const juce::String& text)> onClose;
    };

    explicit DialogOverlay (Options options);

    // Gives keyboard focus to the first useful element. Call once it is on screen.
    void focusFirst();

    void paint (juce::Graphics& g) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress& key) override;
    void mouseDown (const juce::MouseEvent&) override {} // swallow clicks behind the panel

    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

    // On macOS, hides a TextEditor's internal children from VoiceOver so it is not read twice
    // (the same fix as Surge XT's fixupJuceTextEditorAccessibility).
    static void fixTextEditorAccessibility (juce::TextEditor& editor);

private:
    void close (bool accepted);
    juce::Rectangle<int> panelBounds() const;

    Options options;
    juce::Label messageLabel;
    juce::TextEditor messageText;
    juce::TextEditor input;
    juce::TextButton okButton, cancelButton;
    bool closing = false;
};

} // namespace purpletuba
