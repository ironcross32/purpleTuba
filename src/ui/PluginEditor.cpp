// Purple Tuba - the editor.
#include "ui/PluginEditor.h"

#include "ui/KeyboardShortcuts.h"

namespace purpletuba
{

namespace
{
    constexpr int defaultWidth = 1080;
    constexpr int defaultHeight = 640;
    constexpr int headerHeight = 44;
    constexpr int gap = 10;

    juce::String mappingText (const MidiLearn::Mapping& m)
    {
        return "CC " + juce::String (m.controller) + ", channel " + juce::String (m.channel);
    }
}

PurpleTubaEditor::PurpleTubaEditor (PurpleTubaProcessor& p)
    : juce::AudioProcessorEditor (p), processor (p), mappingsList (p.getMidiLearn())
{
    setLookAndFeel (&lookAndFeel);
    setTitle ("Purple Tuba");
    setDescription ("Vocal tract synthesizer. Press F1 for keyboard shortcuts.");
    setFocusContainerType (juce::Component::FocusContainerType::keyboardFocusContainer);
    setWantsKeyboardFocus (true);

    titleLabel.setText ("Purple Tuba", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (24.0f, juce::Font::bold)));
    titleLabel.setColour (juce::Label::textColourId, theme::accent);
    titleLabel.setAccessible (false); // the window / plug-in title already says this
    addAndMakeVisible (titleLabel);

    hintLabel.setText ("F1: keyboard shortcuts.  " + shortcuts::midiLearnText() + ": MIDI learn the focused control.",
                       juce::dontSendNotification);
    hintLabel.setJustificationType (juce::Justification::centredRight);
    hintLabel.setColour (juce::Label::textColourId, theme::dimText);
    addAndMakeVisible (hintLabel);

    addAndMakeVisible (content);

    buildParameterGroups();
    buildLearnGroup();

    lastMappingVersion = processor.getMidiLearn().getVersion();
    syncLearnState();

    setResizable (true, true);
    setResizeLimits (960, 600, 2400, 1600);

    // The resize corner is mouse-only; keep it out of the screen reader's way.
    for (auto* child : getChildren())
        if (dynamic_cast<juce::ResizableCornerComponent*> (child) != nullptr)
            child->setAccessible (false);
    setSize (defaultWidth, defaultHeight);

    startTimerHz (30);
}

PurpleTubaEditor::~PurpleTubaEditor()
{
    stopTimer();

    // Leaving learn armed with no editor to report the result would confuse the next session.
    processor.getMidiLearn().cancel();

    dialog.reset();
    retiredDialog.reset();
    setLookAndFeel (nullptr);
}

void PurpleTubaEditor::buildParameterGroups()
{
    for (int g = 0; g < numGroups; ++g)
    {
        const auto group = static_cast<ParamGroup> (g);
        paramGroups[(size_t) g] = std::make_unique<ControlGroup> (groupName (group), groupDescription (group), g + 1);
        content.addAndMakeVisible (*paramGroups[(size_t) g]);
    }

    for (int i = 0; i < numParams; ++i)
    {
        const auto& info = paramInfo (i);
        auto& group = *paramGroups[(size_t) info.group];

        if (info.kind == ParamKind::toggle)
        {
            auto toggle = std::make_unique<LearnableToggle> (processor, *this, i);
            group.addFullWidth (*toggle, ControlGroup::rowHeight);
            controls[(size_t) i] = std::move (toggle);
        }
        else
        {
            auto slider = std::make_unique<LearnableSlider> (processor, *this, i);
            auto* sliderPtr = slider.get();
            group.addRow (*slider, info.name, true);

            if (auto* valueLabel = group.getValueLabel (*slider))
            {
                auto update = [sliderPtr, valueLabel]
                {
                    valueLabel->setText (sliderPtr->getTextFromValue (sliderPtr->getValue()), juce::dontSendNotification);
                };
                slider->onValueChange = update;
                update();
            }

            controls[(size_t) i] = std::move (slider);
        }
    }
}

void PurpleTubaEditor::buildLearnGroup()
{
    learnGroup = std::make_unique<ControlGroup> ("MIDI learn",
                                                 "MIDI learn status and the list of MIDI mappings",
                                                 numGroups + 1);
    content.addAndMakeVisible (*learnGroup);

    learnStatus.setTitle ("MIDI learn status");
    learnStatus.setJustificationType (juce::Justification::centredLeft);
    learnStatus.setMinimumHorizontalScale (1.0f);
    learnGroup->addFullWidth (learnStatus, 52);

    mappingsList.onMappingRemoved = [this] (const MidiLearn::Mapping& m)
    {
        announcements.enqueue ("Removed the MIDI mapping for " + juce::String (paramInfo (m.param).name) + ".", 1);
        refreshMappings();
    };
    mappingsList.onGoToControl = [this] (int param)
    {
        if (auto* c = getControl (param))
            c->getComponent().grabKeyboardFocus();
    };
    learnGroup->addFullWidth (mappingsList, 150);

    clearAllButton.setDescription ("Removes every MIDI controller mapping.");
    clearAllButton.onClick = [this]
    {
        processor.getMidiLearn().clearAll();
        refreshMappings();
        announcements.enqueue ("All MIDI mappings cleared.", 1);
    };
    learnGroup->addFullWidth (clearAllButton, 36);

    shortcutsButton.setDescription ("Lists the keyboard shortcuts. Shortcut: F1.");
    shortcutsButton.onClick = [this] { showShortcuts(); };
    learnGroup->addFullWidth (shortcutsButton, 36);
}

LearnableControl* PurpleTubaEditor::getControl (int paramIndex) const
{
    if (juce::isPositiveAndBelow (paramIndex, numParams))
        return controls[(size_t) paramIndex].get();
    return nullptr;
}

std::vector<ControlGroup*> PurpleTubaEditor::getGroupsInOrder() const
{
    std::vector<ControlGroup*> result;
    for (const auto& g : paramGroups)
        result.push_back (g.get());
    result.push_back (learnGroup.get());
    return result;
}

//==============================================================================
void PurpleTubaEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);
}

void PurpleTubaEditor::resized()
{
    auto area = getLocalBounds().reduced (gap);

    auto header = area.removeFromTop (headerHeight);
    titleLabel.setBounds (header.removeFromLeft (220));
    hintLabel.setBounds (header);

    area.removeFromTop (4);
    content.setBounds (area);

    // Three columns, filled top to bottom in group order, so reading order matches Tab order.
    auto columns = content.getLocalBounds();
    const int columnWidth = (columns.getWidth() - 2 * gap) / 3;
    auto col1 = columns.removeFromLeft (columnWidth);
    columns.removeFromLeft (gap);
    auto col2 = columns.removeFromLeft (columnWidth);
    columns.removeFromLeft (gap);
    auto col3 = columns;

    auto place = [] (juce::Rectangle<int>& column, ControlGroup& group, bool fill)
    {
        const int h = fill ? column.getHeight() : juce::jmin (column.getHeight(), group.getPreferredHeight());
        group.setBounds (column.removeFromTop (h));
        column.removeFromTop (gap);
    };

    auto& groups = paramGroups;
    place (col1, *groups[(size_t) ParamGroup::voiceSource], false);

    place (col2, *groups[(size_t) ParamGroup::tongue], false);
    place (col2, *groups[(size_t) ParamGroup::constriction1], false);
    place (col2, *groups[(size_t) ParamGroup::constriction2], false);

    place (col3, *groups[(size_t) ParamGroup::nasal], false);
    place (col3, *groups[(size_t) ParamGroup::midiOutput], false);
    place (col3, *learnGroup, true);

    if (dialog != nullptr)
        dialog->setBounds (getLocalBounds());
}

//==============================================================================
void PurpleTubaEditor::visibilityChanged()
{
    focusFirstControlIfNothingFocused();
}

void PurpleTubaEditor::parentHierarchyChanged()
{
    focusFirstControlIfNothingFocused();
}

void PurpleTubaEditor::focusFirstControlIfNothingFocused()
{
    if (initialFocusDone)
        return;

    juce::Component::SafePointer<PurpleTubaEditor> safe (this);
    juce::MessageManager::callAsync ([safe]
    {
        if (safe == nullptr || safe->initialFocusDone || ! safe->isShowing())
            return;

        safe->initialFocusDone = true;
        auto* focused = juce::Component::getCurrentlyFocusedComponent();
        if (focused == nullptr || focused == safe.getComponent() || ! safe->isParentOf (focused))
            if (auto* first = safe->paramGroups.front()->getFirstControl())
                first->grabKeyboardFocus();
    });
}

bool PurpleTubaEditor::keyPressed (const juce::KeyPress& key)
{
    if (dialog != nullptr)
        return false;

    if (shortcuts::isMidiLearn (key))
    {
        toggleMidiLearn();
        return true;
    }
    if (shortcuts::isNextGroup (key))
    {
        moveToGroup (1);
        return true;
    }
    if (shortcuts::isPreviousGroup (key))
    {
        moveToGroup (-1);
        return true;
    }
    if (shortcuts::isHelp (key))
    {
        showShortcuts();
        return true;
    }
    if (key == juce::KeyPress::escapeKey && processor.getMidiLearn().isArmed())
    {
        cancelLearn();
        return true;
    }

    return false;
}

//==============================================================================
void PurpleTubaEditor::toggleMidiLearn()
{
    if (processor.getMidiLearn().isArmed())
    {
        cancelLearn();
        return;
    }

    if (auto* control = LearnableControl::findIn (juce::Component::getCurrentlyFocusedComponent()))
    {
        startLearn (*control);
        return;
    }

    announcements.enqueue ("No control is focused. Move to a control, then press " + shortcuts::midiLearnText() + ".", 1);
}

void PurpleTubaEditor::startLearn (LearnableControl& control)
{
    auto& learn = processor.getMidiLearn();
    learn.arm (control.getParamIndex());
    syncLearnState();

    announcements.enqueue ("MIDI learn for " + juce::String (control.getInfo().name)
                               + ". Move a control on your MIDI device. "
                               + shortcuts::midiLearnText() + " cancels.",
                           1);
}

void PurpleTubaEditor::cancelLearn()
{
    processor.getMidiLearn().cancel();
    syncLearnState();
    announcements.enqueue ("MIDI learn cancelled.", 1);
}

void PurpleTubaEditor::syncLearnState()
{
    const int armed = processor.getMidiLearn().getArmedParam();
    lastArmedParam = armed;

    for (auto& c : controls)
        if (c != nullptr)
            c->setLearnArmed (c->getParamIndex() == armed);

    juce::String status;
    if (armed >= 0)
        status = "MIDI learn: waiting for a MIDI controller for " + juce::String (paramInfo (armed).name) + ".";
    else if (lastLearnedText.isNotEmpty())
        status = "MIDI learn: off. Last learned: " + lastLearnedText + ".";
    else
        status = "MIDI learn: off. Press " + shortcuts::midiLearnText() + " on a control to start.";

    if (learnStatus.getText() != status)
    {
        learnStatus.setText (status, juce::dontSendNotification);
        learnStatus.setColour (juce::Label::textColourId, armed >= 0 ? theme::learnColour : theme::text);
        if (auto* handler = learnStatus.getAccessibilityHandler())
            handler->notifyAccessibilityEvent (juce::AccessibilityEvent::valueChanged);
    }
}

void PurpleTubaEditor::refreshMappings()
{
    lastMappingVersion = processor.getMidiLearn().getVersion();
    mappingsList.refresh();
    for (auto& c : controls)
        if (c != nullptr)
            c->refreshDescription();
}

void PurpleTubaEditor::timerCallback()
{
    auto& learn = processor.getMidiLearn();

    MidiLearn::LearnEvent event;
    while (learn.popLearnEvent (event))
    {
        const juce::String name = paramInfo (event.mapping.param).name;
        lastLearnedText = name + ", " + mappingText (event.mapping);
        announcements.enqueue (name + " learned: " + mappingText (event.mapping) + ".", 1);
    }

    if (learn.getArmedParam() != lastArmedParam || learn.getVersion() != lastMappingVersion)
        syncLearnState();

    if (learn.getVersion() != lastMappingVersion)
        refreshMappings();

    announcements.tick (this);

    if (! sentStructureChange && isShowing())
    {
        sentStructureChange = true;
        if (auto* handler = getAccessibilityHandler())
            handler->notifyAccessibilityEvent (juce::AccessibilityEvent::structureChanged);
    }
}

//==============================================================================
void PurpleTubaEditor::moveToGroup (int delta)
{
    const auto groups = getGroupsInOrder();
    const int count = (int) groups.size();

    int current = -1;
    if (auto* focused = juce::Component::getCurrentlyFocusedComponent())
        for (int i = 0; i < count; ++i)
            if (groups[(size_t) i] == focused || groups[(size_t) i]->isParentOf (focused))
                current = i;

    int next = current < 0 ? (delta > 0 ? 0 : count - 1) : (current + delta + count) % count;

    for (int tries = 0; tries < count; ++tries, next = (next + delta + count) % count)
    {
        if (auto* target = groups[(size_t) next]->getFirstControl())
        {
            target->grabKeyboardFocus();
           #if JUCE_MAC
            // VoiceOver doesn't always say which group focus moved into; NVDA, JAWS and Narrator do.
            announcements.enqueue (groups[(size_t) next]->getTitle(), 2);
           #endif
            return;
        }
    }
}

//==============================================================================
void PurpleTubaEditor::showControlMenu (LearnableControl& control)
{
    auto& learn = processor.getMidiLearn();
    const int index = control.getParamIndex();
    const auto& info = control.getInfo();
    const bool armedHere = learn.getArmedParam() == index;
    const auto mapping = learn.getMappingForParam (index);
    const bool isSlider = info.kind != ParamKind::toggle;

    enum { learnId = 1, clearId, enterId, resetId };

    juce::PopupMenu menu;

    juce::PopupMenu::Item learnItem (armedHere ? "Cancel MIDI learn" : "MIDI learn");
    learnItem.itemID = learnId;
    learnItem.shortcutKeyDescription = shortcuts::midiLearnText();
    menu.addItem (learnItem);

    menu.addItem (clearId,
                  mapping ? "Clear MIDI mapping (" + mappingText (*mapping) + ")" : juce::String ("Clear MIDI mapping (not mapped)"),
                  mapping.has_value());

    menu.addSeparator();
    if (isSlider)
        menu.addItem (enterId, "Enter value...");

    auto& param = control.getParameter();
    menu.addItem (resetId, "Reset to default (" + param.getText (param.getDefaultValue(), 0) + ")");

    juce::Component::SafePointer<juce::Component> safeControl (&control.getComponent());
    juce::Component::SafePointer<PurpleTubaEditor> safeThis (this);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&control.getComponent()),
                        [safeThis, safeControl, index] (int menuResult)
                        {
                            // Act once the menu window has fully gone: focusing the control
                            // immediately loses out to the menu's own focus handling.
                            juce::MessageManager::callAsync ([safeThis, safeControl, index, result = menuResult]
                            {
                                if (safeThis == nullptr)
                                    return;

                                // JUCE still thinks the control has focus, but the OS gave it to the
                                // window while the menu was open. Drop and retake focus so screen
                                // readers get a real focus event and land back on the control.
                                if (safeControl != nullptr && safeControl->isShowing())
                                {
                                    safeControl->giveAwayKeyboardFocus();
                                    if (safeControl != nullptr)
                                        safeControl->grabKeyboardFocus();
                                }

                                auto* control = safeThis->getControl (index);
                                if (control == nullptr)
                                    return;

                                switch (result)
                                {
                                    case learnId:
                                        if (safeThis->processor.getMidiLearn().getArmedParam() == index)
                                            safeThis->cancelLearn();
                                        else
                                            safeThis->startLearn (*control);
                                        break;

                                    case clearId:
                                        safeThis->processor.getMidiLearn().clearMappingForParam (index);
                                        safeThis->refreshMappings();
                                        safeThis->announcements.enqueue ("MIDI mapping cleared for " + juce::String (paramInfo (index).name) + ".", 2);
                                        break;

                                    case enterId:
                                        safeThis->promptForValue (*control);
                                        break;

                                    case resetId:
                                        control->resetToDefault();
                                        break;

                                    default:
                                        break;
                                }
                            });
                        });
}

void PurpleTubaEditor::promptForValue (LearnableControl& control)
{
    const int index = control.getParamIndex();
    const juce::String name = control.getInfo().name;

    DialogOverlay::Options options;
    options.title = "Enter value";
    options.message = name + ". Current value: " + control.getValueText() + ". Type a new value.";
    options.textInput = true;
    options.initialText = control.getValueText().upToFirstOccurrenceOf (",", false, false);
    options.onClose = [this, index] (bool accepted, const juce::String& text)
    {
        closeDialog();
        if (accepted)
            setParameterFromText (index, text);
    };
    showDialog (std::move (options));
}

void PurpleTubaEditor::setParameterFromText (int paramIndex, const juce::String& text)
{
    const auto& info = paramInfo (paramIndex);
    const auto parsed = parseValue (info, text);
    if (! parsed)
    {
        announcements.enqueue ("Could not read \"" + text + "\" as a value for " + info.name + ".", 2);
        return;
    }

    auto* param = processor.getParam (paramIndex);
    param->beginChangeGesture();
    param->setValueNotifyingHost (param->convertTo0to1 (*parsed));
    param->endChangeGesture();
}

void PurpleTubaEditor::showShortcuts()
{
    DialogOverlay::Options options;
    options.title = "Keyboard shortcuts";
    options.message = shortcuts::helpText();
    options.longMessage = true;
    options.okText = "Close";
    options.cancelText = {};
    options.onClose = [this] (bool, const juce::String&) { closeDialog(); };
    showDialog (std::move (options));
}

void PurpleTubaEditor::showDialog (DialogOverlay::Options options)
{
    if (dialog != nullptr)
        return;

    focusBeforeDialog = juce::Component::getCurrentlyFocusedComponent();

    dialog = std::make_unique<DialogOverlay> (std::move (options));
    addAndMakeVisible (*dialog);
    dialog->setBounds (getLocalBounds());

    // Hide everything behind the dialog from screen readers while it is open.
    content.setAccessible (false);
    hintLabel.setAccessible (false);

    dialog->focusFirst();
}

void PurpleTubaEditor::closeDialog()
{
    if (dialog == nullptr)
        return;

    dialog->setVisible (false);
    content.setAccessible (true);
    hintLabel.setAccessible (true);

    if (focusBeforeDialog != nullptr)
        focusBeforeDialog->grabKeyboardFocus();
    else if (auto* first = paramGroups.front()->getFirstControl())
        first->grabKeyboardFocus();

    // Delete the dialog after the current event (we may be inside one of its callbacks).
    retiredDialog = std::move (dialog);
    juce::Component::SafePointer<PurpleTubaEditor> safe (this);
    juce::MessageManager::callAsync ([safe]
    {
        if (safe != nullptr)
            safe->retiredDialog.reset();
    });
}

} // namespace purpletuba
