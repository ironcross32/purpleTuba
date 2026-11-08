#include "midi/MidiLearn.h"

#include <catch2/catch_test_macros.hpp>

using purpletuba::MidiLearn;

TEST_CASE ("Learning maps a controller to the armed parameter", "[midilearn]")
{
    MidiLearn learn (8);
    learn.arm (3);
    REQUIRE (learn.isArmed());

    const auto r = learn.handleController (2, 74);
    CHECK (r.learned);
    CHECK (r.param == 3);
    CHECK_FALSE (learn.isArmed());

    const auto m = learn.getMappingForParam (3);
    REQUIRE (m.has_value());
    CHECK (m->channel == 2);
    CHECK (m->controller == 74);

    MidiLearn::LearnEvent e;
    REQUIRE (learn.popLearnEvent (e));
    CHECK (e.mapping == MidiLearn::Mapping { 3, 2, 74 });
    CHECK_FALSE (learn.popLearnEvent (e));

    // Afterwards the controller drives the parameter, without learning again.
    const auto r2 = learn.handleController (2, 74);
    CHECK_FALSE (r2.learned);
    CHECK (r2.param == 3);

    // Same CC on another channel is not mapped.
    CHECK (learn.handleController (1, 74).param == -1);
}

TEST_CASE ("Cancelling learn leaves mappings untouched", "[midilearn]")
{
    MidiLearn learn (4);
    learn.arm (1);
    learn.cancel();
    CHECK_FALSE (learn.isArmed());
    CHECK (learn.handleController (1, 10).param == -1);
    CHECK_FALSE (learn.getMappingForParam (1).has_value());
}

TEST_CASE ("Bank select and channel mode controllers are never learned", "[midilearn]")
{
    MidiLearn learn (4);
    learn.arm (0);

    for (int cc : { 0, 32, 120, 121, 123, 127 })
    {
        const auto r = learn.handleController (1, cc);
        CHECK_FALSE (r.learned);
        CHECK (learn.isArmed());
    }

    CHECK (learn.handleController (1, 1).learned); // mod wheel is fine
}

TEST_CASE ("A controller belongs to one parameter and a parameter to one controller", "[midilearn]")
{
    MidiLearn learn (4);

    learn.setMapping (0, 1, 20);
    learn.setMapping (1, 1, 20); // steals CC 20 from param 0
    CHECK_FALSE (learn.getMappingForParam (0).has_value());
    CHECK (learn.handleController (1, 20).param == 1);

    learn.setMapping (1, 1, 21); // param 1 moves to CC 21; CC 20 is freed
    CHECK (learn.handleController (1, 20).param == -1);
    CHECK (learn.handleController (1, 21).param == 1);
    CHECK (learn.getAllMappings().size() == 1);
}

TEST_CASE ("Clearing mappings bumps the version", "[midilearn]")
{
    MidiLearn learn (4);
    learn.setMapping (2, 5, 7);
    const auto v = learn.getVersion();

    learn.clearMappingForParam (2);
    CHECK (learn.getVersion() != v);
    CHECK (learn.handleController (5, 7).param == -1);

    learn.setMapping (0, 1, 1);
    learn.setMapping (1, 1, 2);
    learn.clearAll();
    CHECK (learn.getAllMappings().empty());
}

TEST_CASE ("Mappings round-trip through a ValueTree", "[midilearn]")
{
    const char* keys[] = { "a", "b", "c" };
    auto keyFor = [&] (int p) { return juce::String (keys[p]); };
    auto indexFor = [&] (const juce::String& k)
    {
        for (int i = 0; i < 3; ++i)
            if (k == keys[i])
                return i;
        return -1;
    };

    MidiLearn a (3);
    a.setMapping (0, 1, 74);
    a.setMapping (2, 16, 11);
    const auto tree = a.toValueTree (keyFor);

    MidiLearn b (3);
    b.setMapping (1, 3, 3); // replaced by the load
    b.fromValueTree (tree, indexFor);

    CHECK (b.getAllMappings() == a.getAllMappings());
    CHECK_FALSE (b.getMappingForParam (1).has_value());
}
