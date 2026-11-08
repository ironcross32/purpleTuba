// Purple Tuba - parameter controls that can be MIDI-learned.
#include "ui/LearnableControls.h"

#include "ui/KeyboardShortcuts.h"
#include "ui/Theme.h"

namespace purpletuba
{

//==============================================================================
LearnableControl::LearnableControl (PurpleTubaProcessor& p, ControlHost& h, int index)
    : processor (p), host (h), paramIndex (index)
{
}

LearnableControl* LearnableControl::findIn (juce::Component* c)
{
    for (; c != nullptr; c = c->getParentComponent())
        if (auto* lc = dynamic_cast<LearnableControl*> (c))
            return lc;
    return nullptr;
}

void LearnableControl::setLearnArmed (bool shouldBeArmed)
{
    if (learnArmed != shouldBeArmed)
    {
        learnArmed = shouldBeArmed;
        getComponent().repaint();
    }
}

void LearnableControl::refreshDescription()
{
    auto& c = getComponent();
    juce::String text = getInfo().help;

    if (auto m = processor.getMidiLearn().getMappingForParam (paramIndex))
        text << " MIDI: CC " << m->controller << ", channel " << m->channel << ".";
    else
        text << " Not mapped to MIDI.";

    if (c.getDescription() != text)
    {
        c.setDescription (text);
        if (auto* tc = dynamic_cast<juce::SettableTooltipClient*> (&c))
            tc->setTooltip (text);
        if (auto* handler = c.getAccessibilityHandler())
            handler->notifyAccessibilityEvent (juce::AccessibilityEvent::titleChanged);
    }
}

void LearnableControl::resetToDefault()
{
    auto& param = getParameter();
    param.beginChangeGesture();
    param.setValueNotifyingHost (param.getDefaultValue());
    param.endChangeGesture();
}

juce::String LearnableControl::getValueText() const
{
    auto& param = getParameter();
    return param.getCurrentValueAsText();
}

void LearnableControl::paintOutlines (juce::Graphics& g, juce::Component& c) const
{
    auto bounds = c.getLocalBounds().toFloat().reduced (1.0f);

    if (learnArmed)
    {
        g.setColour (theme::learnColour);
        g.drawRoundedRectangle (bounds, 4.0f, 3.0f);
    }
    else if (c.hasKeyboardFocus (false))
    {
        g.setColour (theme::focusColour);
        g.drawRoundedRectangle (bounds, 4.0f, 2.0f);
    }
}

//==============================================================================
LearnableSlider::LearnableSlider (PurpleTubaProcessor& p, ControlHost& h, int index)
    : juce::Slider (juce::Slider::LinearHorizontal, juce::Slider::NoTextBox),
      LearnableControl (p, h, index),
      attachment (p.getValueTreeState(), paramInfo (index).key, *this)
{
    setTitle (getInfo().name);
    setName (getInfo().name);
    setWantsKeyboardFocus (true);
    setPopupMenuEnabled (false);          // the right-click menu is ours
    setScrollWheelEnabled (true);
    setVelocityBasedMode (false);
    setDoubleClickReturnValue (false, 0.0); // double-click opens the value entry instead
    refreshDescription();
}

double LearnableSlider::stepProportion (bool fine) const
{
    const auto& info = getInfo();
    const double span = info.maxValue - info.minValue;
    const double intervalProportion = info.interval > 0.0f && span > 0.0 ? info.interval / span : 0.0;
    return juce::jmax (fine ? 0.001 : 0.01, intervalProportion);
}

void LearnableSlider::nudge (double proportionDelta)
{
    const double current = valueToProportionOfLength (getValue());
    double target = juce::jlimit (0.0, 1.0, current + proportionDelta);

    // Make sure a step on a stepped parameter (e.g. semitones) always moves at least one notch.
    auto newValue = snapValue (proportionOfLengthToValue (target), juce::Slider::notDragging);
    if (juce::approximatelyEqual (newValue, getValue()) && getInterval() > 0.0)
        newValue = juce::jlimit (getMinimum(), getMaximum(), getValue() + (proportionDelta > 0 ? getInterval() : -getInterval()));

    setValue (newValue, juce::sendNotificationSync);
}

bool LearnableSlider::keyPressed (const juce::KeyPress& key)
{
    if (shortcuts::isContextMenu (key))
    {
        host.showControlMenu (*this);
        return true;
    }

    const auto mods = key.getModifiers();
    // Leave Alt/Command/Ctrl combinations (MIDI learn, group navigation) to the editor.
    if (mods.isAltDown() || mods.isCommandDown() || mods.isCtrlDown())
        return false;

    const int code = key.getKeyCode();
    const bool fine = mods.isShiftDown();

    if (code == juce::KeyPress::upKey || code == juce::KeyPress::rightKey)
    {
        nudge (stepProportion (fine));
        return true;
    }
    if (code == juce::KeyPress::downKey || code == juce::KeyPress::leftKey)
    {
        nudge (-stepProportion (fine));
        return true;
    }
    if (code == juce::KeyPress::pageUpKey)
    {
        nudge (0.1);
        return true;
    }
    if (code == juce::KeyPress::pageDownKey)
    {
        nudge (-0.1);
        return true;
    }
    if (code == juce::KeyPress::homeKey)
    {
        setValue (getMaximum(), juce::sendNotificationSync);
        return true;
    }
    if (code == juce::KeyPress::endKey)
    {
        setValue (getMinimum(), juce::sendNotificationSync);
        return true;
    }
    if (code == juce::KeyPress::deleteKey || code == juce::KeyPress::backspaceKey)
    {
        resetToDefault();
        return true;
    }
    if (code == juce::KeyPress::returnKey)
    {
        host.promptForValue (*this);
        return true;
    }

    return false;
}

void LearnableSlider::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        grabKeyboardFocus();
        host.showControlMenu (*this);
        return;
    }
    juce::Slider::mouseDown (e);
}

void LearnableSlider::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu())
        host.promptForValue (*this);
}

void LearnableSlider::paint (juce::Graphics& g)
{
    juce::Slider::paint (g);
    paintOutlines (g, *this);
}

//==============================================================================
LearnableToggle::LearnableToggle (PurpleTubaProcessor& p, ControlHost& h, int index)
    : juce::ToggleButton (paramInfo (index).name),
      LearnableControl (p, h, index),
      attachment (p.getValueTreeState(), paramInfo (index).key, *this)
{
    setTitle (getInfo().name);
    setName (getInfo().name);
    setWantsKeyboardFocus (true);
    refreshDescription();
}

bool LearnableToggle::keyPressed (const juce::KeyPress& key)
{
    if (shortcuts::isContextMenu (key))
    {
        host.showControlMenu (*this);
        return true;
    }

    const auto mods = key.getModifiers();
    if (mods.isAltDown() || mods.isCommandDown() || mods.isCtrlDown())
        return false;

    const int code = key.getKeyCode();
    if (code == juce::KeyPress::deleteKey || code == juce::KeyPress::backspaceKey)
    {
        resetToDefault();
        return true;
    }

    return juce::ToggleButton::keyPressed (key);
}

void LearnableToggle::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        grabKeyboardFocus();
        host.showControlMenu (*this);
        return;
    }
    juce::ToggleButton::mouseDown (e);
}

void LearnableToggle::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    juce::ToggleButton::paintButton (g, highlighted, down);
    paintOutlines (g, *this);
}

} // namespace purpletuba
