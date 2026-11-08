// Purple Tuba - the complete voice.
// Follows the AudioSystem of Pink Trombone 1.1 (c) 2017 Neil Thapen, MIT license;
// see THIRD_PARTY_NOTICES.md.
#include "dsp/VoiceEngine.h"

#include <algorithm>
#include <cmath>

namespace purpletuba::dsp
{

namespace
{
    constexpr double fricativeAttackSeconds = 0.1; // the original's fricativeAttackTime
}

VoiceEngine::VoiceEngine()
{
    reset();
}

void VoiceEngine::reset()
{
    glottis.prepare (internalSampleRate);
    tract.prepare (internalSampleRate);
    aspirateFilter.setup (internalSampleRate, 500.0, 0.5);
    fricativeFilter.setup (internalSampleRate, 1000.0, 0.5);
    aspirateFilter.reset();
    fricativeFilter.reset();
    fricativeIntensity.fill (0.0);
    noteHeld = false;
    hasNotePitch = false;
    currentGain = params.outputGain;
    blockPos = blockSize;
    applyControls();
}

void VoiceEngine::noteOn (double frequencyHz) noexcept
{
    noteFrequency = frequencyHz;
    noteHeld = true;
    hasNotePitch = true;
    applyControls();
    glottis.snapPitchIfSilent();
}

double VoiceEngine::constrictionIndex (double position) noexcept
{
    return Tract::minConstrictionIndex
         + std::clamp (position, 0.0, 1.0) * (Tract::maxConstrictionIndex - Tract::minConstrictionIndex);
}

double VoiceEngine::constrictionTouchDiameter (double closure) noexcept
{
    // closure 0 -> touch diameter 3.3 (just outside the original's effective range, so no effect)
    // closure 1 -> 0.3 (fully closed). Squared so the fricative region (touch diameter 0.3..0.7)
    // gets about a third of the control's travel instead of a few MIDI steps.
    const double open = 1.0 - std::clamp (closure, 0.0, 1.0);
    return 0.3 + 3.0 * open * open;
}

double VoiceEngine::tongueDiameterFromHeight (double height) noexcept
{
    return Tract::outerTongueControlRadius
         - std::clamp (height, 0.0, 1.0) * (Tract::outerTongueControlRadius - Tract::innerTongueControlRadius);
}

double VoiceEngine::tongueIndexFromPosition (double position, double height) noexcept
{
    double lo, hi;
    Tract::tongueIndexRange (tongueDiameterFromHeight (height), lo, hi);
    return lo + std::clamp (position, 0.0, 1.0) * (hi - lo);
}

void VoiceEngine::applyControls() noexcept
{
    const double bendRatio = std::pow (2.0, bendSemitones / 12.0);

    // With a note held (or releasing) follow the note; otherwise the Pitch control drives the voice.
    double baseFrequency = params.pitchHz;
    if (noteHeld || (hasNotePitch && ! params.alwaysVoice))
        baseFrequency = noteFrequency;

    Glottis::Controls gc;
    gc.frequency = std::clamp (baseFrequency * bendRatio, 20.0, 4000.0);
    gc.tenseness = params.tenseness;
    gc.loudness = params.loudness;
    gc.breathiness = params.breathiness;
    gc.vibratoAmount = params.vibratoDepth;
    gc.vibratoFrequency = params.vibratoRate;
    gc.glideSecondsPerOctave = params.glideSecondsPerOctave;
    gc.pitchWobble = params.pitchWobble;
    gc.alwaysVoice = params.alwaysVoice;
    gc.voiced = noteHeld;
    glottis.setControls (gc);

    // Tract shape: tongue first, then the constrictions (TractUI.handleTouches).
    const double tongueDiameter = tongueDiameterFromHeight (params.tongueHeight);
    tract.setTongue (tongueIndexFromPosition (params.tonguePosition, params.tongueHeight), tongueDiameter);
    tract.resetTargetToRest();

    std::array<Tract::TurbulenceSource, numConstrictions> sources {};
    int numSources = 0;

    for (size_t i = 0; i < (size_t) numConstrictions; ++i)
    {
        const auto& c = params.constrictions[i];
        const bool active = constrictionActive (c.closure);
        const double index = constrictionIndex (c.position);
        const double touchDiameter = constrictionTouchDiameter (c.closure);

        if (active)
            tract.applyConstriction (index, touchDiameter);

        // Fricative intensity ramps like a touch starting / ending in the original.
        const double target = active ? std::clamp (c.fricative, 0.0, 1.0) : 0.0;
        const double step = blockSeconds / fricativeAttackSeconds;
        auto& fi = fricativeIntensity[i];
        fi = fi < target ? std::min (fi + step, target) : std::max (fi - step, target);

        if (fi > 0.0)
            sources[(size_t) numSources++] = { index, touchDiameter, fi };
    }

    tract.setTurbulenceSources (sources.data(), numSources);
    tract.setVelumTarget (velumFromNasal (std::clamp (params.nasal, 0.0, 1.0)));
}

void VoiceEngine::renderBlock() noexcept
{
    applyControls();

    const double targetGain = params.outputGain;
    const double gainStep = (targetGain - currentGain) / blockSize;

    for (int j = 0; j < blockSize; ++j)
    {
        const double lambda1 = static_cast<double> (j) / blockSize;
        const double lambda2 = (j + 0.5) / blockSize;

        const float white = whiteNoise.next();
        const double aspiration = aspirateFilter.process (white);
        const double fricative = fricativeFilter.process (white);

        const double glottalOutput = glottis.runStep (lambda1, aspiration);
        const double noiseModulator = glottis.getNoiseModulator();

        // the tract runs at twice the sample rate
        double vocalOutput = 0.0;
        tract.runStep (glottalOutput, fricative, noiseModulator, lambda1);
        vocalOutput += tract.getLipOutput() + tract.getNoseOutput();
        tract.runStep (glottalOutput, fricative, noiseModulator, lambda2);
        vocalOutput += tract.getLipOutput() + tract.getNoseOutput();

        currentGain += gainStep;
        block[(size_t) j] = static_cast<float> (vocalOutput * 0.125 * currentGain);
    }
    currentGain = targetGain;

    glottis.finishBlock (blockSeconds);
    tract.finishBlock (blockSeconds);

    // Safety net: never let a numerical blow-up reach the host.
    for (auto& s : block)
    {
        if (! std::isfinite (s))
        {
            block.fill (0.0f);
            tract.prepare (internalSampleRate);
            break;
        }
        s = std::clamp (s, -4.0f, 4.0f);
    }
}

void VoiceEngine::render (float* out, int numSamples) noexcept
{
    int written = 0;
    while (written < numSamples)
    {
        if (blockPos >= blockSize)
        {
            renderBlock();
            blockPos = 0;
        }

        const int n = std::min (numSamples - written, blockSize - blockPos);
        std::copy (block.begin() + blockPos, block.begin() + blockPos + n, out + written);
        written += n;
        blockPos += n;
    }
}

} // namespace purpletuba::dsp
