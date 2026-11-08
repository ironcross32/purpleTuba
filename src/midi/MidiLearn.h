// Purple Tuba - MIDI learn: CC -> parameter mappings, safe to use from the audio thread.
//
// The editor arms learn for one parameter. The audio thread calls handleController() for every
// incoming CC: when armed it records the mapping and queues a LearnEvent for the editor to
// announce; otherwise it returns the parameter the CC is mapped to. All state is atomics, so
// the audio thread never locks.
#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <array>
#include <atomic>
#include <functional>
#include <optional>
#include <vector>

namespace purpletuba
{

class MidiLearn
{
public:
    static constexpr int numChannels = 16;
    static constexpr int numControllers = 128;
    static constexpr int numSlots = numChannels * numControllers;

    struct Mapping
    {
        int param = -1;
        int channel = 1;    // 1..16
        int controller = 0; // 0..127
        bool operator== (const Mapping&) const = default;
    };

    struct LearnEvent
    {
        Mapping mapping;
    };

    struct ControllerResult
    {
        int param = -1;       // parameter to set from this CC, or -1
        bool learned = false; // this CC just completed MIDI learn
    };

    explicit MidiLearn (int numParams);

    // Bank select and channel-mode messages are never learned.
    static bool isLearnableController (int controller) noexcept;

    // --- Message thread ---
    void arm (int param) noexcept;
    void cancel() noexcept;
    int getArmedParam() const noexcept { return armed.load (std::memory_order_acquire); }
    bool isArmed() const noexcept { return getArmedParam() >= 0; }

    void setMapping (int param, int channel, int controller) noexcept;
    void clearMappingForParam (int param) noexcept;
    void clearAll() noexcept;
    std::optional<Mapping> getMappingForParam (int param) const noexcept;
    std::vector<Mapping> getAllMappings() const;

    // Increments whenever any mapping changes, so the UI can refresh cheaply.
    uint32_t getVersion() const noexcept { return version.load (std::memory_order_acquire); }

    // Takes the next learn event, if any (single consumer: the editor).
    bool popLearnEvent (LearnEvent& e) noexcept;

    // --- Audio thread ---
    ControllerResult handleController (int channel, int controller) noexcept;

    // --- State ---
    juce::ValueTree toValueTree (const std::function<juce::String (int)>& paramKey) const;
    void fromValueTree (const juce::ValueTree& tree, const std::function<int (const juce::String&)>& paramIndex);

    static constexpr const char* stateTag = "MidiMappings";

private:
    static int slotFor (int channel, int controller) noexcept { return (channel - 1) * numControllers + controller; }
    void assign (int param, int slot) noexcept;
    void pushEvent (const LearnEvent& e) noexcept;

    const int numParams;
    std::atomic<int> armed { -1 };
    std::array<std::atomic<int>, numSlots> slotToParam;
    std::vector<std::atomic<int>> paramToSlot;
    std::atomic<uint32_t> version { 0 };

    // Single-producer (audio) / single-consumer (editor) ring of learn events.
    static constexpr int eventCapacity = 16;
    std::array<LearnEvent, eventCapacity> events {};
    std::atomic<int> eventWrite { 0 }, eventRead { 0 };
};

} // namespace purpletuba
