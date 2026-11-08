// Purple Tuba - audio processor.
#include "PluginProcessor.h"

#include "ui/PluginEditor.h"

#include <cmath>

namespace purpletuba
{

namespace
{
    constexpr int maxResampleChunk = 512;
}

PurpleTubaProcessor::PurpleTubaProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PurpleTuba", createParameterLayout())
{
    for (int i = 0; i < numParams; ++i)
    {
        const auto& info = paramInfo (i);
        params[(size_t) i] = apvts.getParameter (info.key);
        rawValues[(size_t) i] = apvts.getRawParameterValue (info.key);
        jassert (params[(size_t) i] != nullptr && rawValues[(size_t) i] != nullptr);
    }
}

bool PurpleTubaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet().isDisabled();
}

void PurpleTubaProcessor::prepareToPlay (double sampleRate, int)
{
    hostSampleRate = sampleRate > 0 ? sampleRate : dsp::VoiceEngine::internalSampleRate;
    resampleRatio = dsp::VoiceEngine::internalSampleRate / hostSampleRate;
    resampling = std::abs (resampleRatio - 1.0) > 1.0e-9;

    interpolator.reset();
    pending.assign ((size_t) (std::ceil (maxResampleChunk * resampleRatio) + 64), 0.0f);
    pendingCount = 0;

    numHeldNotes = 0;
    pitchBendNormalised = 0.0f;
    engine.reset();
    updateEngineParams();

    // The windowed-sinc interpolator delays the signal by about half its length (in input samples).
    setLatencySamples (resampling ? juce::roundToInt (juce::WindowedSincInterpolator::getBaseLatency() / resampleRatio) : 0);
}

double PurpleTubaProcessor::noteFrequency (int note) const noexcept
{
    return 440.0 * std::pow (2.0, (note - 69) / 12.0);
}

void PurpleTubaProcessor::updateEngineParams() noexcept
{
    dsp::VoiceParams p;
    p.pitchHz = value (ParamId::pitch);
    p.tenseness = value (ParamId::tenseness);
    p.loudness = value (ParamId::loudness);
    p.breathiness = value (ParamId::breathiness);
    p.vibratoDepth = value (ParamId::vibratoDepth);
    p.vibratoRate = value (ParamId::vibratoRate);
    p.pitchWobble = value (ParamId::pitchWobble) >= 0.5f;
    p.alwaysVoice = value (ParamId::alwaysVoice) >= 0.5f;
    p.tonguePosition = value (ParamId::tonguePosition);
    p.tongueHeight = value (ParamId::tongueHeight);
    p.constrictions[0] = { value (ParamId::c1Position), value (ParamId::c1Closure), value (ParamId::c1Fricative) };
    p.constrictions[1] = { value (ParamId::c2Position), value (ParamId::c2Closure), value (ParamId::c2Fricative) };
    p.nasal = value (ParamId::nasal);
    p.glideSecondsPerOctave = value (ParamId::glide);

    const float db = value (ParamId::outputLevel);
    p.outputGain = db <= -59.9f ? 0.0 : std::pow (10.0, db / 20.0);

    engine.setParams (p);
    engine.setPitchBendSemitones (pitchBendNormalised * value (ParamId::bendRange));
}

void PurpleTubaProcessor::applyControllerValue (int paramIndex, int controllerValue)
{
    auto* param = getParam (paramIndex);
    if (param == nullptr)
        return;

    const auto& info = paramInfo (paramIndex);
    const float normalised = info.kind == ParamKind::toggle ? (controllerValue >= 64 ? 1.0f : 0.0f)
                                                            : (float) juce::jlimit (0, 127, controllerValue) / 127.0f;

    // JUCE's VST3 and CLAP wrappers queue parameter changes made on the audio thread.
    if (! juce::approximatelyEqual (param->getValue(), normalised))
        param->setValueNotifyingHost (normalised);
}

void PurpleTubaProcessor::handleMidiMessage (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
    {
        const int note = m.getNoteNumber();
        // Move the note to the top of the stack.
        int w = 0;
        for (int r = 0; r < numHeldNotes; ++r)
            if (heldNotes[(size_t) r] != note)
                heldNotes[(size_t) w++] = heldNotes[(size_t) r];
        numHeldNotes = w;
        if (numHeldNotes < (int) heldNotes.size())
            heldNotes[(size_t) numHeldNotes++] = note;

        engine.noteOn (noteFrequency (note));
    }
    else if (m.isNoteOff())
    {
        const int note = m.getNoteNumber();
        const bool wasTop = numHeldNotes > 0 && heldNotes[(size_t) numHeldNotes - 1] == note;
        int w = 0;
        for (int r = 0; r < numHeldNotes; ++r)
            if (heldNotes[(size_t) r] != note)
                heldNotes[(size_t) w++] = heldNotes[(size_t) r];
        numHeldNotes = w;

        if (numHeldNotes == 0)
            engine.noteOff();
        else if (wasTop)
            engine.setNoteFrequency (noteFrequency (heldNotes[(size_t) numHeldNotes - 1]));
    }
    else if (m.isPitchWheel())
    {
        const int v = m.getPitchWheelValue() - 8192;
        pitchBendNormalised = v >= 0 ? (float) v / 8191.0f : (float) v / 8192.0f;
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        numHeldNotes = 0;
        engine.noteOff();
    }
    else if (m.isController())
    {
        const auto result = midiLearn.handleController (m.getChannel(), m.getControllerNumber());
        if (result.param >= 0)
            applyControllerValue (result.param, m.getControllerValue());
    }
}

void PurpleTubaProcessor::renderHost (float* out, int numSamples) noexcept
{
    if (! resampling)
    {
        engine.render (out, numSamples);
        return;
    }

    while (numSamples > 0)
    {
        const int chunk = std::min (numSamples, maxResampleChunk);
        const int needed = std::min ((int) std::ceil (chunk * resampleRatio) + 2, (int) pending.size());

        if (pendingCount < needed)
        {
            engine.render (pending.data() + pendingCount, needed - pendingCount);
            pendingCount = needed;
        }

        const int used = juce::jlimit (0, pendingCount, interpolator.process (resampleRatio, pending.data(), out, chunk));
        std::copy (pending.begin() + used, pending.begin() + pendingCount, pending.begin());
        pendingCount -= used;

        out += chunk;
        numSamples -= chunk;
    }
}

void PurpleTubaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    buffer.clear();
    if (numSamples == 0 || buffer.getNumChannels() == 0)
        return;

    updateEngineParams();

    auto* out = buffer.getWritePointer (0);
    int position = 0;

    for (const auto metadata : midi)
    {
        const int eventPos = juce::jlimit (0, numSamples, metadata.samplePosition);
        if (eventPos > position)
        {
            renderHost (out + position, eventPos - position);
            position = eventPos;
        }

        handleMidiMessage (metadata.getMessage());
        updateEngineParams();
    }

    if (position < numSamples)
        renderHost (out + position, numSamples - position);

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
}

void PurpleTubaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    for (int i = state.getNumChildren(); --i >= 0;)
        if (state.getChild (i).hasType (MidiLearn::stateTag))
            state.removeChild (i, nullptr);

    state.appendChild (midiLearn.toValueTree ([] (int p) { return juce::String (paramInfo (p).key); }), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void PurpleTubaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    auto tree = juce::ValueTree::fromXml (*xml);
    const auto mappings = tree.getChildWithName (MidiLearn::stateTag);
    midiLearn.fromValueTree (mappings, [] (const juce::String& key) { return paramIndexForKey (key); });
    if (mappings.isValid())
        tree.removeChild (mappings, nullptr);

    apvts.replaceState (tree);
}

juce::AudioProcessorEditor* PurpleTubaProcessor::createEditor()
{
    return new PurpleTubaEditor (*this);
}

} // namespace purpletuba

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new purpletuba::PurpleTubaProcessor();
}
