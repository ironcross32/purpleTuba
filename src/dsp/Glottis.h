// Purple Tuba - glottal source (Liljencrants-Fant waveform with vibrato and aspiration).
// Port of the Glottis object from Pink Trombone 1.1 (c) 2017 Neil Thapen, MIT license;
// see THIRD_PARTY_NOTICES.md.
#pragma once

#include "dsp/Noise.h"

namespace purpletuba::dsp
{

class Glottis
{
public:
    struct Controls
    {
        double frequency = 140.0;            // target pitch in Hz ("UIFrequency")
        double tenseness = 0.6;              // 0 = breathy/whisper, 1 = pressed ("UITenseness")
        double loudness = 1.0;               // user scale on top of the tenseness-derived loudness
        double breathiness = 1.0;            // aspiration gain (1 = original)
        double vibratoAmount = 0.005;        // fractional pitch deviation
        double vibratoFrequency = 6.0;       // Hz
        double glideSecondsPerOctave = 0.078; // 0 = jump; the original moves ~0.078 s per octave
        bool pitchWobble = true;             // "pitch wobble" button in the original
        bool alwaysVoice = false;            // "always voice" button in the original
        bool voiced = false;                 // a note is held (the original's "isTouched")
    };

    void prepare (double sampleRate);
    void reset();

    void setControls (const Controls& c) noexcept;

    // If the voice is silent, jump straight to the new pitch instead of gliding,
    // as the original does when a touch starts while intensity == 0.
    void snapPitchIfSilent() noexcept;

    // One sample. lambda = position within the current control block (0..1).
    double runStep (double lambda, double aspirationNoise) noexcept;

    // Called once at the end of each control block of length blockSeconds.
    void finishBlock (double blockSeconds) noexcept;

    double getNoiseModulator() const noexcept;
    double getIntensity() const noexcept { return intensity; }
    double getCurrentFrequency() const noexcept { return smoothFrequency; }

private:
    void setupWaveform (double lambda) noexcept;
    double normalizedLFWaveform (double t) const noexcept;

    Controls controls;
    SimplexNoise noise;

    double sampleRate = 48000.0;
    double timeInWaveform = 0.0;
    double totalTime = 0.0;
    double oldFrequency = 140.0, newFrequency = 140.0, smoothFrequency = 140.0;
    double oldTenseness = 0.6, newTenseness = 0.6;
    double intensity = 0.0;
    double loudnessGain = 1.0; // tenseness^0.25 * controls.loudness, as in the original

    // LF waveform state (from setupWaveform)
    double frequency = 140.0, waveformLength = 1.0 / 140.0;
    double alpha = 0, E0 = 0, epsilon = 0, shift = 0, Delta = 1, Te = 0, omega = 0;
};

} // namespace purpletuba::dsp
