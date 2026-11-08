#include "TestHelpers.h"

#include "PluginProcessor.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <set>

using namespace purpletuba;

TEST_CASE ("The parameter table is complete and consistent", "[params]")
{
    std::set<juce::String> keys;
    for (int i = 0; i < numParams; ++i)
    {
        const auto& p = paramInfo (i);
        INFO (p.key);
        CHECK ((int) p.id == i);
        CHECK (juce::String (p.name).isNotEmpty());
        CHECK (juce::String (p.help).isNotEmpty());
        CHECK (p.minValue < p.maxValue);
        CHECK (p.defaultValue >= p.minValue);
        CHECK (p.defaultValue <= p.maxValue);
        CHECK (keys.insert (p.key).second);
        CHECK (paramIndexForKey (p.key) == i);
    }
    CHECK (paramIndexForKey ("noSuchParameter") == -1);
}

TEST_CASE ("Every group has a name and at least one control", "[params]")
{
    for (int g = 0; g < numGroups; ++g)
    {
        const auto group = static_cast<ParamGroup> (g);
        CHECK (groupName (group).isNotEmpty());
        CHECK (groupDescription (group).isNotEmpty());

        int count = 0;
        for (const auto& p : allParams())
            count += p.group == group ? 1 : 0;
        CHECK (count > 0);
    }
}

TEST_CASE ("Every parameter exists in the processor and can be MIDI learned", "[params]")
{
    testhelpers::ensureJuceInitialised();
    PurpleTubaProcessor processor;

    for (int i = 0; i < numParams; ++i)
    {
        const auto& info = paramInfo (i);
        INFO (info.key);
        auto* param = processor.getParam (i);
        REQUIRE (param != nullptr);
        CHECK (param->getName (100) == info.name);

        // Learn it from a controller and drive it to both ends.
        auto& learn = processor.getMidiLearn();
        learn.arm (i);
        const int cc = 1 + (i % 100);
        const auto result = learn.handleController (1, cc);
        REQUIRE (result.learned);
        REQUIRE (result.param == i);

        processor.applyControllerValue (i, 127);
        CHECK (param->getValue() == 1.0f);
        processor.applyControllerValue (i, 0);
        CHECK (param->getValue() == 0.0f);
    }
}

TEST_CASE ("Value text parses back to (about) the same value", "[params]")
{
    for (const auto& p : allParams())
    {
        INFO (p.key);
        const float span = p.maxValue - p.minValue;
        for (float t : { 0.0f, 0.13f, 0.5f, 0.77f, 1.0f })
        {
            float v = p.minValue + t * span;
            if (p.kind == ParamKind::toggle)
                v = t >= 0.5f ? 1.0f : 0.0f;
            if (p.interval > 0.0f)
                v = p.minValue + std::round ((v - p.minValue) / p.interval) * p.interval;

            const auto text = formatValue (p, v);
            const auto parsed = parseValue (p, text);
            INFO (text.toStdString());
            REQUIRE (parsed.has_value());
            CHECK (std::abs (*parsed - v) <= juce::jmax (0.011f * span, 1.0e-4f) + (p.format == ValueFormat::pitchHz ? 0.5f : 0.0f));
        }
    }

    CHECK_FALSE (parseValue (paramInfo (ParamId::pitch), "loud").has_value());
    CHECK (*parseValue (paramInfo (ParamId::glide), "1.5 s") == 1.5f);
    CHECK (*parseValue (paramInfo (ParamId::glide), "200 ms") == Catch::Approx (0.2f));
    CHECK (*parseValue (paramInfo (ParamId::tenseness), "250") == 1.0f); // clamped
}

TEST_CASE ("Value text is descriptive", "[params]")
{
    CHECK (formatValue (paramInfo (ParamId::pitch), 440.0f) == "440 Hz, A4");
    CHECK (formatValue (paramInfo (ParamId::c1Closure), 0.0f) == "0 %, off");
    CHECK (formatValue (paramInfo (ParamId::c1Closure), 1.0f) == "100 %, closed");
    CHECK (formatValue (paramInfo (ParamId::c1Closure), 0.8f).endsWith ("fricative"));
    CHECK (formatValue (paramInfo (ParamId::c2Position), 1.0f).endsWith ("lips"));
    CHECK (formatValue (paramInfo (ParamId::alwaysVoice), 1.0f) == "On");
}
