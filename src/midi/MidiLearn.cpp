// Purple Tuba - MIDI learn.
#include "midi/MidiLearn.h"

namespace purpletuba
{

MidiLearn::MidiLearn (int nParams)
    : numParams (nParams), paramToSlot ((size_t) nParams)
{
    for (auto& s : slotToParam)
        s.store (-1);
    for (auto& p : paramToSlot)
        p.store (-1);
}

bool MidiLearn::isLearnableController (int controller) noexcept
{
    if (controller < 0 || controller >= numControllers)
        return false;
    if (controller == 0 || controller == 32) // bank select MSB / LSB
        return false;
    if (controller >= 120)                   // channel mode messages (all notes off etc.)
        return false;
    return true;
}

void MidiLearn::arm (int param) noexcept
{
    if (param >= 0 && param < numParams)
        armed.store (param, std::memory_order_release);
}

void MidiLearn::cancel() noexcept
{
    armed.store (-1, std::memory_order_release);
}

void MidiLearn::assign (int param, int slot) noexcept
{
    // Remove the parameter's previous controller.
    const int oldSlot = paramToSlot[(size_t) param].exchange (slot, std::memory_order_acq_rel);
    if (oldSlot >= 0 && oldSlot != slot)
    {
        int expected = param;
        slotToParam[(size_t) oldSlot].compare_exchange_strong (expected, -1, std::memory_order_acq_rel);
    }

    // Take the controller away from whichever parameter had it.
    const int previousParam = slotToParam[(size_t) slot].exchange (param, std::memory_order_acq_rel);
    if (previousParam >= 0 && previousParam != param)
    {
        int expected = slot;
        paramToSlot[(size_t) previousParam].compare_exchange_strong (expected, -1, std::memory_order_acq_rel);
    }

    version.fetch_add (1, std::memory_order_acq_rel);
}

void MidiLearn::setMapping (int param, int channel, int controller) noexcept
{
    if (param < 0 || param >= numParams || channel < 1 || channel > numChannels
        || ! isLearnableController (controller))
        return;
    assign (param, slotFor (channel, controller));
}

void MidiLearn::clearMappingForParam (int param) noexcept
{
    if (param < 0 || param >= numParams)
        return;

    const int slot = paramToSlot[(size_t) param].exchange (-1, std::memory_order_acq_rel);
    if (slot >= 0)
    {
        int expected = param;
        slotToParam[(size_t) slot].compare_exchange_strong (expected, -1, std::memory_order_acq_rel);
        version.fetch_add (1, std::memory_order_acq_rel);
    }
}

void MidiLearn::clearAll() noexcept
{
    for (int p = 0; p < numParams; ++p)
        paramToSlot[(size_t) p].store (-1, std::memory_order_release);
    for (auto& s : slotToParam)
        s.store (-1, std::memory_order_release);
    version.fetch_add (1, std::memory_order_acq_rel);
}

std::optional<MidiLearn::Mapping> MidiLearn::getMappingForParam (int param) const noexcept
{
    if (param < 0 || param >= numParams)
        return std::nullopt;

    const int slot = paramToSlot[(size_t) param].load (std::memory_order_acquire);
    if (slot < 0)
        return std::nullopt;

    return Mapping { param, slot / numControllers + 1, slot % numControllers };
}

std::vector<MidiLearn::Mapping> MidiLearn::getAllMappings() const
{
    std::vector<Mapping> result;
    for (int p = 0; p < numParams; ++p)
        if (auto m = getMappingForParam (p))
            result.push_back (*m);
    return result;
}

MidiLearn::ControllerResult MidiLearn::handleController (int channel, int controller) noexcept
{
    if (channel < 1 || channel > numChannels || controller < 0 || controller >= numControllers)
        return {};

    const int slot = slotFor (channel, controller);

    int armedParam = armed.load (std::memory_order_acquire);
    if (armedParam >= 0 && isLearnableController (controller)
        && armed.compare_exchange_strong (armedParam, -1, std::memory_order_acq_rel))
    {
        assign (armedParam, slot);
        pushEvent ({ Mapping { armedParam, channel, controller } });
        return { armedParam, true };
    }

    return { slotToParam[(size_t) slot].load (std::memory_order_acquire), false };
}

void MidiLearn::pushEvent (const LearnEvent& e) noexcept
{
    const int w = eventWrite.load (std::memory_order_relaxed);
    const int next = (w + 1) % eventCapacity;
    if (next == eventRead.load (std::memory_order_acquire))
        return; // full; the editor is not draining. The mapping itself is still stored.
    events[(size_t) w] = e;
    eventWrite.store (next, std::memory_order_release);
}

bool MidiLearn::popLearnEvent (LearnEvent& e) noexcept
{
    const int r = eventRead.load (std::memory_order_relaxed);
    if (r == eventWrite.load (std::memory_order_acquire))
        return false;
    e = events[(size_t) r];
    eventRead.store ((r + 1) % eventCapacity, std::memory_order_release);
    return true;
}

juce::ValueTree MidiLearn::toValueTree (const std::function<juce::String (int)>& paramKey) const
{
    juce::ValueTree tree (stateTag);
    for (const auto& m : getAllMappings())
    {
        juce::ValueTree child ("Mapping");
        child.setProperty ("param", paramKey (m.param), nullptr);
        child.setProperty ("channel", m.channel, nullptr);
        child.setProperty ("cc", m.controller, nullptr);
        tree.appendChild (child, nullptr);
    }
    return tree;
}

void MidiLearn::fromValueTree (const juce::ValueTree& tree, const std::function<int (const juce::String&)>& paramIndex)
{
    clearAll();
    if (! tree.hasType (stateTag))
        return;

    for (const auto& child : tree)
    {
        const int p = paramIndex (child.getProperty ("param").toString());
        const int ch = child.getProperty ("channel", 1);
        const int cc = child.getProperty ("cc", -1);
        setMapping (p, ch, cc);
    }
}

} // namespace purpletuba
