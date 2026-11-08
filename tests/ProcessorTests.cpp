#include "TestHelpers.h"

#include "PluginProcessor.h"

#include <catch2/catch_test_macros.hpp>

#include <vector>

using namespace purpletuba;

namespace
{
    void setParam (PurpleTubaProcessor& p, ParamId id, float value)
    {
        auto* param = p.getParam (id);
        param->setValueNotifyingHost (param->convertTo0to1 (value));
    }

    // Renders `seconds` of audio in host-sized blocks, returning channel 0 (and checking channel 1).
    std::vector<float> renderSeconds (PurpleTubaProcessor& p, double sampleRate, double seconds,
                                      juce::MidiBuffer firstBlockMidi = {})
    {
        constexpr int blockSize = 256;
        const int total = (int) (seconds * sampleRate);
        std::vector<float> out;
        out.reserve ((size_t) total);

        juce::AudioBuffer<float> buffer (2, blockSize);
        bool first = true;
        for (int done = 0; done < total; done += blockSize)
        {
            juce::MidiBuffer midi;
            if (first)
                midi = firstBlockMidi;
            first = false;

            p.processBlock (buffer, midi);
            for (int i = 0; i < blockSize; ++i)
            {
                out.push_back (buffer.getSample (0, i));
                REQUIRE (buffer.getSample (1, i) == buffer.getSample (0, i));
            }
        }
        return out;
    }

    void steadyVoice (PurpleTubaProcessor& p)
    {
        setParam (p, ParamId::alwaysVoice, 1.0f);
        setParam (p, ParamId::pitchWobble, 0.0f);
        setParam (p, ParamId::vibratoDepth, 0.0f);
    }
}

TEST_CASE ("The processor makes sound at common host sample rates", "[processor]")
{
    testhelpers::ensureJuceInitialised();

    for (double sr : { 44100.0, 48000.0, 88200.0, 96000.0 })
    {
        INFO (sr);
        PurpleTubaProcessor p;
        steadyVoice (p);
        p.prepareToPlay (sr, 256);
        const auto out = renderSeconds (p, sr, 1.0);
        REQUIRE (testhelpers::allFinite (out));
        CHECK (testhelpers::rms (out, out.size() / 2) > 0.01);
    }
}

TEST_CASE ("The voice sounds the same at every host sample rate (fixed internal rate)", "[processor]")
{
    testhelpers::ensureJuceInitialised();

    auto strongestHarmonic = [] (double sr)
    {
        PurpleTubaProcessor p;
        steadyVoice (p);
        p.prepareToPlay (sr, 256);
        const auto out = renderSeconds (p, sr, 1.2);
        // Analyse 0.25 s from the steady part.
        const auto start = (size_t) (0.8 * sr);
        std::vector<float> window (out.begin() + (long) start, out.begin() + (long) (start + (size_t) (0.25 * sr)));
        return testhelpers::peakFrequency (window, sr, 150.0, 2500.0, 5.0);
    };

    const double at48 = strongestHarmonic (48000.0);
    CHECK (std::abs (strongestHarmonic (44100.0) - at48) <= 10.0);
    CHECK (std::abs (strongestHarmonic (96000.0) - at48) <= 10.0);
}

TEST_CASE ("MIDI notes gate the voice", "[processor]")
{
    testhelpers::ensureJuceInitialised();
    PurpleTubaProcessor p;
    p.prepareToPlay (48000.0, 256);

    CHECK (testhelpers::rms (renderSeconds (p, 48000.0, 0.3)) < 1.0e-6);

    juce::MidiBuffer on;
    on.addEvent (juce::MidiMessage::noteOn (1, 57, (juce::uint8) 100), 0);
    const auto held = renderSeconds (p, 48000.0, 0.6, on);
    CHECK (testhelpers::rms (held, held.size() / 2) > 0.01);

    juce::MidiBuffer off;
    off.addEvent (juce::MidiMessage::noteOff (1, 57), 0);
    const auto released = renderSeconds (p, 48000.0, 1.5, off);
    CHECK (testhelpers::rms (released, released.size() - 12000) < 1.0e-3);
}

TEST_CASE ("MIDI learn through processBlock", "[processor][midilearn]")
{
    testhelpers::ensureJuceInitialised();
    PurpleTubaProcessor p;
    p.prepareToPlay (48000.0, 256);
    auto& learn = p.getMidiLearn();
    const int tongue = (int) ParamId::tonguePosition;
    const int height = (int) ParamId::tongueHeight;

    learn.arm (tongue);
    {
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::controllerEvent (1, 74, 100), 10);
        renderSeconds (p, 48000.0, 0.01, midi);
    }

    MidiLearn::LearnEvent e;
    REQUIRE (learn.popLearnEvent (e));
    CHECK (e.mapping == MidiLearn::Mapping { tongue, 1, 74 });
    CHECK_FALSE (learn.isArmed());
    CHECK (std::abs (p.getParam (tongue)->getValue() - 100.0f / 127.0f) < 1.0e-5f);

    {
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::controllerEvent (1, 74, 0), 0);
        renderSeconds (p, 48000.0, 0.01, midi);
    }
    CHECK (p.getParam (tongue)->getValue() == 0.0f);
    CHECK_FALSE (learn.popLearnEvent (e));

    // Learning the same CC for another parameter moves it.
    learn.arm (height);
    {
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::controllerEvent (1, 74, 127), 0);
        renderSeconds (p, 48000.0, 0.01, midi);
    }
    CHECK_FALSE (learn.getMappingForParam (tongue).has_value());
    REQUIRE (learn.getMappingForParam (height).has_value());
    CHECK (p.getParam (height)->getValue() == 1.0f);

    // A cancelled learn records nothing.
    learn.arm (tongue);
    learn.cancel();
    {
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::controllerEvent (1, 20, 64), 0);
        renderSeconds (p, 48000.0, 0.01, midi);
    }
    CHECK_FALSE (learn.getMappingForParam (tongue).has_value());
}

TEST_CASE ("A learned controller switches toggles at the half-way point", "[processor][midilearn]")
{
    testhelpers::ensureJuceInitialised();
    PurpleTubaProcessor p;
    p.prepareToPlay (48000.0, 256);
    const int toggle = (int) ParamId::alwaysVoice;
    p.getMidiLearn().setMapping (toggle, 1, 64);

    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::controllerEvent (1, 64, 127), 0);
    renderSeconds (p, 48000.0, 0.01, midi);
    CHECK (p.getParam (toggle)->getValue() == 1.0f);

    juce::MidiBuffer midi2;
    midi2.addEvent (juce::MidiMessage::controllerEvent (1, 64, 10), 0);
    renderSeconds (p, 48000.0, 0.01, midi2);
    CHECK (p.getParam (toggle)->getValue() == 0.0f);
}

TEST_CASE ("State saves and restores parameters and MIDI mappings", "[processor][state]")
{
    testhelpers::ensureJuceInitialised();

    juce::MemoryBlock state;
    {
        PurpleTubaProcessor a;
        setParam (a, ParamId::tongueHeight, 0.25f);
        setParam (a, ParamId::pitch, 220.0f);
        a.getMidiLearn().setMapping ((int) ParamId::c1Closure, 3, 21);
        a.getMidiLearn().setMapping ((int) ParamId::nasal, 1, 1);
        a.getStateInformation (state);

        // Saving twice must not duplicate the mappings.
        a.getStateInformation (state);
    }

    PurpleTubaProcessor b;
    b.getMidiLearn().setMapping ((int) ParamId::loudness, 1, 7); // should be replaced
    b.setStateInformation (state.getData(), (int) state.getSize());

    CHECK (std::abs (b.getParam (ParamId::tongueHeight)->convertFrom0to1 (b.getParam (ParamId::tongueHeight)->getValue()) - 0.25f) < 1.0e-4f);
    CHECK (std::abs (b.getParam (ParamId::pitch)->convertFrom0to1 (b.getParam (ParamId::pitch)->getValue()) - 220.0f) < 0.05f);

    const auto mappings = b.getMidiLearn().getAllMappings();
    REQUIRE (mappings.size() == 2);
    CHECK (b.getMidiLearn().getMappingForParam ((int) ParamId::c1Closure) == MidiLearn::Mapping { (int) ParamId::c1Closure, 3, 21 });
    CHECK (b.getMidiLearn().getMappingForParam ((int) ParamId::nasal) == MidiLearn::Mapping { (int) ParamId::nasal, 1, 1 });
    CHECK_FALSE (b.getMidiLearn().getMappingForParam ((int) ParamId::loudness).has_value());

    // And the state from b (after a load) still has exactly one mappings element.
    juce::MemoryBlock again;
    b.getStateInformation (again);
    auto xml = juce::AudioProcessor::getXmlFromBinary (again.getData(), (int) again.getSize());
    REQUIRE (xml != nullptr);
    int count = 0;
    for (auto* child : xml->getChildIterator())
        count += child->hasTagName (MidiLearn::stateTag) ? 1 : 0;
    CHECK (count == 1);
}
