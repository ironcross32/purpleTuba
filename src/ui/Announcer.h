// Purple Tuba - screen reader announcements.
//
// On Windows, JUCE's AccessibilityHandler::postAnnouncement speaks through SAPI, which talks over
// the user's screen reader in a different voice. Instead we raise a UI Automation notification
// event, so NVDA / JAWS / Narrator speak the text themselves with the user's own voice and
// settings. macOS uses JUCE's implementation (a native NSAccessibility announcement).
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <deque>

namespace purpletuba::a11y
{

// Speaks text right away. source is used to find the window to raise the event from.
// Safe to call from any thread (it hops to the message thread).
void announce (const juce::String& text, juce::Component* source = nullptr);

// Queues announcements and posts them a few timer ticks later, so the focus or value change that
// caused them is spoken first and does not cut them off (the same idea as Surge XT's
// enqueueAccessibleAnnouncement). Message thread only.
class AnnouncementQueue
{
public:
    void enqueue (const juce::String& text, int delayTicks = 3);
    void clear() { queue.clear(); }

    // Call from a timer (about 30 Hz).
    void tick (juce::Component* source);

private:
    struct Item { juce::String text; int ticksLeft; };
    std::deque<Item> queue;
};

} // namespace purpletuba::a11y
