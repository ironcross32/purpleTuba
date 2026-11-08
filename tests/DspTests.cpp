#include "TestHelpers.h"

#include "dsp/VoiceEngine.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

using purpletuba::dsp::Tract;
using purpletuba::dsp::VoiceEngine;
using purpletuba::dsp::VoiceParams;
using testhelpers::allFinite;
using testhelpers::rms;

namespace
{
    std::vector<float> render (VoiceEngine& engine, double seconds)
    {
        std::vector<float> out ((size_t) (seconds * VoiceEngine::internalSampleRate));
        // Render in odd-sized chunks to exercise the partial-block path.
        size_t pos = 0;
        int chunk = 37;
        while (pos < out.size())
        {
            const int n = (int) std::min<size_t> ((size_t) chunk, out.size() - pos);
            engine.render (out.data() + pos, n);
            pos += (size_t) n;
            chunk = chunk == 37 ? 500 : 37;
        }
        return out;
    }

    VoiceParams steadyVoice()
    {
        VoiceParams p;
        p.alwaysVoice = true;
        p.pitchWobble = false;
        p.vibratoDepth = 0.0;
        return p;
    }
}

TEST_CASE ("Always voice produces finite, audible output", "[dsp]")
{
    VoiceEngine engine;
    engine.setParams (steadyVoice());
    const auto out = render (engine, 1.0);

    REQUIRE (allFinite (out));
    CHECK (rms (out, out.size() / 2) > 0.01);
}

TEST_CASE ("Without notes or Always voice the engine is silent", "[dsp]")
{
    VoiceEngine engine;
    VoiceParams p;
    p.alwaysVoice = false;
    engine.setParams (p);
    const auto out = render (engine, 0.5);
    CHECK (rms (out) < 1.0e-6);
}

TEST_CASE ("Notes gate the voice", "[dsp]")
{
    VoiceEngine engine;
    VoiceParams p;
    p.alwaysVoice = false;
    engine.setParams (p);

    engine.noteOn (220.0);
    const auto held = render (engine, 0.6);
    CHECK (rms (held, held.size() / 2) > 0.01);

    engine.noteOff();
    const auto released = render (engine, 1.5);
    REQUIRE (allFinite (released));
    // The last quarter second after release should have decayed to (near) silence.
    CHECK (rms (released, released.size() - 12000) < 1.0e-3);
}

TEST_CASE ("Closing the lips muffles the voice and the nasal opening lets it out", "[dsp]")
{
    auto measure = [] (double lipClosure, double nasal)
    {
        VoiceEngine engine;
        auto p = steadyVoice();
        p.constrictions[1].position = (41.0 - 2.0) / 41.0; // lips
        p.constrictions[1].closure = lipClosure;
        p.constrictions[1].fricative = 0.0;
        p.breathiness = 0.0;
        p.nasal = nasal;
        engine.setParams (p);
        const auto out = render (engine, 1.0);
        REQUIRE (allFinite (out));
        return rms (out, out.size() / 2);
    };

    const double open = measure (0.0, 0.0);
    const double closed = measure (1.0, 0.0);
    const double hum = measure (1.0, 1.0);

    CHECK (closed < open * 0.3);
    CHECK (hum > closed * 2.0);
}

TEST_CASE ("A narrow constriction makes fricative noise", "[dsp]")
{
    auto measure = [] (double fricative)
    {
        VoiceEngine engine;
        VoiceParams p;
        p.alwaysVoice = true;
        p.loudness = 0.0;    // no voiced sound
        p.breathiness = 0.0; // no glottal aspiration
        p.constrictions[0].position = (36.0 - 2.0) / 41.0;
        p.constrictions[0].closure = 0.8; // inside the fricative band
        p.constrictions[0].fricative = fricative;
        engine.setParams (p);
        const auto out = render (engine, 1.0);
        REQUIRE (allFinite (out));
        return rms (out, out.size() / 2);
    };

    const double hiss = measure (1.0);
    const double none = measure (0.0);
    CHECK (hiss > 1.0e-4);
    CHECK (hiss > none * 5.0);
}

TEST_CASE ("Control mappings stay inside the original's ranges", "[dsp]")
{
    CHECK (VoiceEngine::constrictionTouchDiameter (0.0) == Catch::Approx (3.3));
    CHECK (VoiceEngine::constrictionTouchDiameter (1.0) == Catch::Approx (0.3));
    CHECK_FALSE (VoiceEngine::constrictionActive (0.0));
    CHECK (VoiceEngine::constrictionActive (0.01));

    CHECK (VoiceEngine::constrictionIndex (0.0) == Catch::Approx (Tract::minConstrictionIndex));
    CHECK (VoiceEngine::constrictionIndex (1.0) == Catch::Approx (Tract::maxConstrictionIndex));

    CHECK (VoiceEngine::tongueDiameterFromHeight (0.0) == Catch::Approx (Tract::outerTongueControlRadius));
    CHECK (VoiceEngine::tongueDiameterFromHeight (1.0) == Catch::Approx (Tract::innerTongueControlRadius));

    for (double h = 0.0; h <= 1.0; h += 0.1)
        for (double pos = 0.0; pos <= 1.0; pos += 0.1)
        {
            const double index = VoiceEngine::tongueIndexFromPosition (pos, h);
            CHECK (index >= Tract::tongueLowerIndexBound - 1.0e-9);
            CHECK (index <= Tract::tongueUpperIndexBound + 1.0e-9);
        }

    CHECK (VoiceEngine::velumFromNasal (0.0) == Catch::Approx (0.01));
    CHECK (VoiceEngine::velumFromNasal (1.0) == Catch::Approx (0.4));
}

TEST_CASE ("Extreme settings never produce NaN or runaway output", "[dsp]")
{
    VoiceEngine engine;
    VoiceParams p;
    p.alwaysVoice = true;
    p.pitchHz = 1000.0;
    p.tenseness = 1.0;
    p.loudness = 1.0;
    p.breathiness = 2.0;
    p.vibratoDepth = 0.05;
    p.vibratoRate = 12.0;
    p.tonguePosition = 1.0;
    p.tongueHeight = 1.0;
    p.constrictions[0] = { 0.0, 0.95, 1.0 };
    p.constrictions[1] = { 1.0, 0.9, 1.0 };
    p.nasal = 1.0;
    p.glideSecondsPerOctave = 0.0;
    p.outputGain = 4.0;
    engine.setParams (p);

    const auto out = render (engine, 2.0);
    REQUIRE (allFinite (out));
    for (auto s : out)
        REQUIRE (std::abs (s) <= 4.0f);
}
