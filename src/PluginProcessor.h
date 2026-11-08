// Purple Tuba - audio processor.
#pragma once

#include "Parameters.h"
#include "dsp/VoiceEngine.h"
#include "midi/MidiLearn.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <vector>

namespace purpletuba
{

class PurpleTubaProcessor final : public juce::AudioProcessor
{
public:
    PurpleTubaProcessor();
    ~PurpleTubaProcessor() override = default;

    // AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Purple Tuba"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Purple Tuba
    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return apvts; }
    MidiLearn& getMidiLearn() noexcept { return midiLearn; }
    juce::RangedAudioParameter* getParam (int index) const noexcept { return params[(size_t) index]; }
    juce::RangedAudioParameter* getParam (ParamId id) const noexcept { return getParam ((int) id); }

    // Sets a parameter from a MIDI controller value (0..127), as MIDI learn does.
    void applyControllerValue (int paramIndex, int controllerValue);

private:
    void handleMidiMessage (const juce::MidiMessage& m);
    void updateEngineParams() noexcept;
    void renderHost (float* out, int numSamples) noexcept;
    double noteFrequency (int note) const noexcept;
    float value (ParamId id) const noexcept { return rawValues[(size_t) id]->load (std::memory_order_relaxed); }

    juce::AudioProcessorValueTreeState apvts;
    std::array<juce::RangedAudioParameter*, numParams> params {};
    std::array<std::atomic<float>*, numParams> rawValues {};

    MidiLearn midiLearn { numParams };
    dsp::VoiceEngine engine;

    // Note stack for last-note priority.
    std::array<int, 128> heldNotes {};
    int numHeldNotes = 0;
    float pitchBendNormalised = 0.0f; // -1..1

    // Resampling from the engine's fixed 48 kHz to the host rate.
    double hostSampleRate = dsp::VoiceEngine::internalSampleRate;
    double resampleRatio = 1.0; // internal samples per host sample
    bool resampling = false;
    juce::WindowedSincInterpolator interpolator;
    std::vector<float> pending;
    int pendingCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PurpleTubaProcessor)
};

} // namespace purpletuba
