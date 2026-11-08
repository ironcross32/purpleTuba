// Purple Tuba - the single table of parameters.
//
// Everything about a parameter (ID, accessible name, group, range, default, value text, help
// text) lives here. The APVTS layout, the editor's grouped controls, the accessible names and
// descriptions, and MIDI learn are all generated from this table, so a control cannot be left
// out of MIDI learn or the screen reader order.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <optional>

namespace purpletuba
{

// Group order is also the Tab / screen reader order.
enum class ParamGroup
{
    voiceSource,
    tongue,
    constriction1,
    constriction2,
    nasal,
    midiOutput,
    count
};

enum class ParamId : int
{
    pitch,
    tenseness,
    loudness,
    breathiness,
    vibratoDepth,
    vibratoRate,
    pitchWobble,
    alwaysVoice,

    tonguePosition,
    tongueHeight,

    c1Position,
    c1Closure,
    c1Fricative,

    c2Position,
    c2Closure,
    c2Fricative,

    nasal,

    glide,
    bendRange,
    outputLevel,

    count
};

constexpr int numParams = static_cast<int> (ParamId::count);
constexpr int numGroups = static_cast<int> (ParamGroup::count);

enum class ParamKind { continuous, toggle };

enum class ValueFormat
{
    pitchHz,
    percent,
    vibratoDepth,
    rateHz,
    tonguePosition,
    tongueHeight,
    tractPosition,
    closure,
    nasal,
    secondsPerOctave,
    semitones,
    decibels,
    onOff
};

struct ParamInfo
{
    ParamId id;
    const char* key;          // stable APVTS / host ID
    const char* name;         // accessible title and visible label
    ParamGroup group;
    ParamKind kind;
    float minValue, maxValue, defaultValue;
    float interval;           // 0 = continuous
    float skewCentre;         // 0 = linear
    ValueFormat format;
    const char* help;         // accessible description / tooltip
};

const std::array<ParamInfo, numParams>& allParams();
const ParamInfo& paramInfo (ParamId id);
inline const ParamInfo& paramInfo (int index) { return allParams()[(size_t) index]; }
int paramIndexForKey (const juce::String& key); // -1 if unknown

juce::String groupName (ParamGroup g);
juce::String groupDescription (ParamGroup g);

juce::NormalisableRange<float> rangeFor (const ParamInfo& p);
juce::String formatValue (const ParamInfo& p, float value);
std::optional<float> parseValue (const ParamInfo& p, const juce::String& text);

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

} // namespace purpletuba
