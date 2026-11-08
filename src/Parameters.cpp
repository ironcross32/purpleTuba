// Purple Tuba - the single table of parameters.
#include "Parameters.h"

#include "dsp/Tract.h"
#include "dsp/VoiceEngine.h"

#include <cmath>

namespace purpletuba
{

namespace
{
    using G = ParamGroup;
    using K = ParamKind;
    using F = ValueFormat;
    using P = ParamId;

    // Default constriction positions: tongue tip (index 36) and lips (index 41).
    constexpr float tipDefault = (36.0f - 2.0f) / 41.0f;
    constexpr float lipDefault = (41.0f - 2.0f) / 41.0f;

    const std::array<ParamInfo, numParams> table { {
        { P::pitch, "pitch", "Pitch", G::voiceSource, K::continuous, 40.0f, 1000.0f, 140.0f, 0.0f, 200.0f, F::pitchHz,
          "Pitch of the voice when no MIDI note is held. Used with Always voice, or for playing without a keyboard." },
        { P::tenseness, "tenseness", "Tenseness", G::voiceSource, K::continuous, 0.0f, 1.0f, 0.6f, 0.0f, 0.0f, F::percent,
          "How tightly the vocal folds close. Low is breathy and whispered, high is pressed and bright. Also affects loudness." },
        { P::loudness, "loudness", "Loudness", G::voiceSource, K::continuous, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, F::percent,
          "Strength of the voiced sound, like breath pressure." },
        { P::breathiness, "breathiness", "Breathiness", G::voiceSource, K::continuous, 0.0f, 2.0f, 1.0f, 0.0f, 0.0f, F::percent,
          "Amount of aspiration noise from the glottis. 100 percent matches the original." },
        { P::vibratoDepth, "vibratoDepth", "Vibrato depth", G::voiceSource, K::continuous, 0.0f, 0.05f, 0.005f, 0.0f, 0.0f, F::vibratoDepth,
          "Depth of the regular vibrato, as a percentage of the pitch." },
        { P::vibratoRate, "vibratoRate", "Vibrato rate", G::voiceSource, K::continuous, 0.0f, 12.0f, 6.0f, 0.0f, 0.0f, F::rateHz,
          "Speed of the regular vibrato." },
        { P::pitchWobble, "pitchWobble", "Pitch wobble", G::voiceSource, K::toggle, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, F::onOff,
          "Adds slow random pitch drift, like the pitch wobble button in the original." },
        { P::alwaysVoice, "alwaysVoice", "Always voice", G::voiceSource, K::toggle, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, F::onOff,
          "Keeps the voice sounding without MIDI notes, using the Pitch control." },

        { P::tonguePosition, "tonguePosition", "Tongue position", G::tongue, K::continuous, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, F::tonguePosition,
          "Moves the tongue body from the back of the mouth to the front. Has less effect when the tongue is low." },
        { P::tongueHeight, "tongueHeight", "Tongue height", G::tongue, K::continuous, 0.0f, 1.0f, 0.738f, 0.0f, 0.0f, F::tongueHeight,
          "Raises the tongue toward the roof of the mouth. Low and back gives ah, high and front gives ee." },

        { P::c1Position, "c1Position", "Constriction 1 position", G::constriction1, K::continuous, 0.0f, 1.0f, tipDefault, 0.0f, 0.0f, F::tractPosition,
          "Where along the tract constriction 1 is placed, from the throat to the lips." },
        { P::c1Closure, "c1Closure", "Constriction 1 closure", G::constriction1, K::continuous, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, F::closure,
          "How narrow constriction 1 is. 0 is off. Narrow settings hiss (fricatives), 100 percent closes the tract; reopening it makes a plosive." },
        { P::c1Fricative, "c1Fricative", "Constriction 1 fricative noise", G::constriction1, K::continuous, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, F::percent,
          "Amount of turbulence noise when constriction 1 is narrow." },

        { P::c2Position, "c2Position", "Constriction 2 position", G::constriction2, K::continuous, 0.0f, 1.0f, lipDefault, 0.0f, 0.0f, F::tractPosition,
          "Where along the tract constriction 2 is placed, from the throat to the lips." },
        { P::c2Closure, "c2Closure", "Constriction 2 closure", G::constriction2, K::continuous, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, F::closure,
          "How narrow constriction 2 is. 0 is off. Narrow settings hiss (fricatives), 100 percent closes the tract; reopening it makes a plosive." },
        { P::c2Fricative, "c2Fricative", "Constriction 2 fricative noise", G::constriction2, K::continuous, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, F::percent,
          "Amount of turbulence noise when constriction 2 is narrow." },

        { P::nasal, "nasal", "Nasal opening", G::nasal, K::continuous, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, F::nasal,
          "Opens the velum so sound also passes through the nose, for m, n and ng sounds." },

        { P::glide, "glide", "Glide time", G::midiOutput, K::continuous, 0.0f, 2.0f, 0.078f, 0.0f, 0.25f, F::secondsPerOctave,
          "How quickly the pitch slides to a new note, in time per octave. 0 jumps instantly." },
        { P::bendRange, "bendRange", "Pitch bend range", G::midiOutput, K::continuous, 0.0f, 24.0f, 2.0f, 1.0f, 0.0f, F::semitones,
          "Range of the MIDI pitch bend wheel in semitones." },
        { P::outputLevel, "outputLevel", "Output level", G::midiOutput, K::continuous, -60.0f, 12.0f, 0.0f, 0.0f, 0.0f, F::decibels,
          "Overall output volume." },
    } };

    // Whole percentages, or one decimal for fine (Shift+arrow) steps so every step is audible.
    juce::String percentText (float v)
    {
        const float pct = v * 100.0f;
        if (std::abs (pct - std::round (pct)) < 0.05f)
            return juce::String (juce::roundToInt (pct)) + " %";
        return juce::String (pct, 1) + " %";
    }

    juce::String tractRegion (double index)
    {
        if (index < purpletuba::dsp::Tract::bladeStart) return "throat";
        if (index < 25.0) return "soft palate";
        if (index < purpletuba::dsp::Tract::tipStart) return "hard palate";
        if (index < purpletuba::dsp::Tract::lipStart) return "tongue tip";
        return "lips";
    }

    // Leading number of a string, e.g. "140 Hz" -> 140. Returns nullopt if there is none.
    std::optional<double> leadingNumber (const juce::String& text)
    {
        auto t = text.trim();
        if (t.isEmpty())
            return std::nullopt;

        const auto numeric = t.initialSectionContainingOnly ("+-0123456789.,");
        if (numeric.isEmpty() || ! numeric.containsAnyOf ("0123456789"))
            return std::nullopt;

        return numeric.replaceCharacter (',', '.').getDoubleValue();
    }
}

const std::array<ParamInfo, numParams>& allParams() { return table; }

const ParamInfo& paramInfo (ParamId id) { return table[(size_t) id]; }

int paramIndexForKey (const juce::String& key)
{
    for (size_t i = 0; i < table.size(); ++i)
        if (key == table[i].key)
            return (int) i;
    return -1;
}

juce::String groupName (ParamGroup g)
{
    switch (g)
    {
        case ParamGroup::voiceSource:   return "Voice source";
        case ParamGroup::tongue:        return "Tongue";
        case ParamGroup::constriction1: return "Constriction 1";
        case ParamGroup::constriction2: return "Constriction 2";
        case ParamGroup::nasal:         return "Nasal";
        case ParamGroup::midiOutput:    return "MIDI and output";
        case ParamGroup::count:         break;
    }
    return {};
}

juce::String groupDescription (ParamGroup g)
{
    switch (g)
    {
        case ParamGroup::voiceSource:   return "The vocal folds: pitch, tone and vibrato";
        case ParamGroup::tongue:        return "Tongue shape, which sets the vowel";
        case ParamGroup::constriction1: return "A narrowing of the tract for consonants";
        case ParamGroup::constriction2: return "A second narrowing of the tract, at the lips by default";
        case ParamGroup::nasal:         return "Airflow through the nose";
        case ParamGroup::midiOutput:    return "Glide, pitch bend, output level and MIDI learn";
        case ParamGroup::count:         break;
    }
    return {};
}

juce::NormalisableRange<float> rangeFor (const ParamInfo& p)
{
    juce::NormalisableRange<float> r (p.minValue, p.maxValue, p.interval);
    if (p.skewCentre > 0.0f)
        r.setSkewForCentre (p.skewCentre);
    return r;
}

juce::String formatValue (const ParamInfo& p, float v)
{
    switch (p.format)
    {
        case ValueFormat::pitchHz:
        {
            const int note = juce::roundToInt (69.0 + 12.0 * std::log2 (std::max (1.0f, v) / 440.0));
            return juce::String (juce::roundToInt (v)) + " Hz, " + juce::MidiMessage::getMidiNoteName (note, true, true, 4);
        }

        case ValueFormat::percent:
            return percentText (v);

        case ValueFormat::vibratoDepth:
            return juce::String (v * 100.0f, 2) + " %";

        case ValueFormat::rateHz:
            return juce::String (v, 1) + " Hz";

        case ValueFormat::tonguePosition:
            return percentText (v) + (v < 0.34f ? ", back" : v < 0.67f ? ", middle" : ", front");

        case ValueFormat::tongueHeight:
            return percentText (v) + (v < 0.34f ? ", low" : v < 0.67f ? ", mid" : ", high");

        case ValueFormat::tractPosition:
            return percentText (v) + ", " + tractRegion (dsp::VoiceEngine::constrictionIndex (v));

        case ValueFormat::closure:
        {
            if (! dsp::VoiceEngine::constrictionActive (v))
                return "0 %, off";
            if (v >= 0.9995f)
                return "100 %, closed";
            const double touch = dsp::VoiceEngine::constrictionTouchDiameter (v);
            const auto pct = percentText (v);
            if (touch < 0.7)
                return pct + ", fricative";
            return pct + ", narrowed";
        }

        case ValueFormat::nasal:
            return v <= 0.0005f ? juce::String ("0 %, closed") : percentText (v);

        case ValueFormat::secondsPerOctave:
            if (v <= 0.0005f)
                return "0 ms, instant";
            if (v < 1.0f)
                return juce::String (juce::roundToInt (v * 1000.0f)) + " ms per octave";
            return juce::String (v, 2) + " s per octave";

        case ValueFormat::semitones:
        {
            const int s = juce::roundToInt (v);
            return juce::String (s) + (s == 1 ? " semitone" : " semitones");
        }

        case ValueFormat::decibels:
            return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " dB";

        case ValueFormat::onOff:
            return v >= 0.5f ? "On" : "Off";
    }
    return juce::String (v);
}

std::optional<float> parseValue (const ParamInfo& p, const juce::String& text)
{
    const auto lower = text.trim().toLowerCase();
    std::optional<double> result;

    if (p.format == ValueFormat::onOff)
    {
        if (lower == "on" || lower == "yes" || lower == "true" || lower == "1") result = 1.0;
        else if (lower == "off" || lower == "no" || lower == "false" || lower == "0") result = 0.0;
    }
    else if (auto num = leadingNumber (lower))
    {
        switch (p.format)
        {
            case ValueFormat::percent:
            case ValueFormat::vibratoDepth:
            case ValueFormat::tonguePosition:
            case ValueFormat::tongueHeight:
            case ValueFormat::tractPosition:
            case ValueFormat::closure:
            case ValueFormat::nasal:
                result = *num / 100.0;
                break;

            case ValueFormat::secondsPerOctave:
                // "78", "78 ms" -> milliseconds; "1.2 s" -> seconds
                result = (lower.contains ("ms") || ! lower.contains ("s")) ? *num / 1000.0 : *num;
                break;

            case ValueFormat::pitchHz:
            case ValueFormat::rateHz:
            case ValueFormat::semitones:
            case ValueFormat::decibels:
            case ValueFormat::onOff:
                result = *num;
                break;
        }
    }

    if (! result)
        return std::nullopt;

    return juce::jlimit (p.minValue, p.maxValue, static_cast<float> (*result));
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (const auto& p : table)
    {
        const juce::ParameterID pid { p.key, 1 };
        const auto* info = &p;

        if (p.kind == ParamKind::toggle)
        {
            layout.add (std::make_unique<juce::AudioParameterBool> (
                pid, p.name, p.defaultValue >= 0.5f,
                juce::AudioParameterBoolAttributes()
                    .withStringFromValueFunction ([] (bool v, int) { return juce::String (v ? "On" : "Off"); })
                    .withValueFromStringFunction ([info] (const juce::String& s)
                                                  { return parseValue (*info, s).value_or (info->defaultValue) >= 0.5f; })));
        }
        else
        {
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                pid, p.name, rangeFor (p), p.defaultValue,
                juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction ([info] (float v, int) { return formatValue (*info, v); })
                    .withValueFromStringFunction ([info] (const juce::String& s)
                                                  { return parseValue (*info, s).value_or (info->defaultValue); })));
        }
    }

    return layout;
}

} // namespace purpletuba
