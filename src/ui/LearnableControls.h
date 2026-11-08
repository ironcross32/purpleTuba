// Purple Tuba - parameter controls that can be MIDI-learned.
//
// Standard JUCE Slider / ToggleButton (so JUCE's own accessibility handlers do the heavy lifting),
// plus Surge XT-style keyboard editing, a context menu on Shift+F10 / the Menu key / right-click,
// a visible focus outline, and a visible MIDI-learn outline.
#pragma once

#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace purpletuba
{

class LearnableControl;

// Implemented by the editor.
struct ControlHost
{
    virtual ~ControlHost() = default;
    virtual void showControlMenu (LearnableControl& control) = 0;
    virtual void promptForValue (LearnableControl& control) = 0;
};

class LearnableControl
{
public:
    LearnableControl (PurpleTubaProcessor& p, ControlHost& h, int paramIndex);
    virtual ~LearnableControl() = default;

    int getParamIndex() const noexcept { return paramIndex; }
    const ParamInfo& getInfo() const noexcept { return paramInfo (paramIndex); }
    juce::RangedAudioParameter& getParameter() const noexcept { return *processor.getParam (paramIndex); }

    virtual juce::Component& getComponent() = 0;

    void setLearnArmed (bool shouldBeArmed);
    bool isLearnArmed() const noexcept { return learnArmed; }

    // Rebuilds the accessible description / tooltip, including the current MIDI mapping.
    void refreshDescription();

    void resetToDefault();
    juce::String getValueText() const;

    // Finds the LearnableControl that is c or contains c.
    static LearnableControl* findIn (juce::Component* c);

protected:
    void paintOutlines (juce::Graphics& g, juce::Component& c) const;

    PurpleTubaProcessor& processor;
    ControlHost& host;

private:
    const int paramIndex;
    bool learnArmed = false;
};

class LearnableSlider final : public juce::Slider, public LearnableControl
{
public:
    LearnableSlider (PurpleTubaProcessor& p, ControlHost& h, int paramIndex);

    juce::Component& getComponent() override { return *this; }

    bool keyPressed (const juce::KeyPress& key) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;
    void paint (juce::Graphics& g) override;
    void focusGained (FocusChangeType) override { repaint(); }
    void focusLost (FocusChangeType) override { repaint(); }

private:
    void nudge (double proportionDelta);
    double stepProportion (bool fine) const;

    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

class LearnableToggle final : public juce::ToggleButton, public LearnableControl
{
public:
    LearnableToggle (PurpleTubaProcessor& p, ControlHost& h, int paramIndex);

    juce::Component& getComponent() override { return *this; }

    bool keyPressed (const juce::KeyPress& key) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void paintButton (juce::Graphics& g, bool highlighted, bool down) override;
    void focusGained (FocusChangeType) override { repaint(); }
    void focusLost (FocusChangeType) override { repaint(); }

private:
    juce::AudioProcessorValueTreeState::ButtonAttachment attachment;
};

} // namespace purpletuba
