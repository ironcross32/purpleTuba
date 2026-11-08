// Purple Tuba - the complete voice: glottis -> tract (+nose), with aspiration/fricative noise.
// Follows the AudioSystem of Pink Trombone 1.1 (c) 2017 Neil Thapen, MIT license;
// see THIRD_PARTY_NOTICES.md.
//
// The engine always runs at a fixed internal rate (48 kHz) because the tract's formants depend
// on its step rate. The plug-in processor resamples to the host rate.
#pragma once

#include "dsp/Glottis.h"
#include "dsp/Noise.h"
#include "dsp/Tract.h"

#include <array>

namespace purpletuba::dsp
{

// User-facing control values. Normalised controls are 0..1.
struct VoiceParams
{
    // Voice source
    double pitchHz = 140.0;
    double tenseness = 0.6;
    double loudness = 1.0;
    double breathiness = 1.0;
    double vibratoDepth = 0.005;   // fraction of pitch
    double vibratoRate = 6.0;      // Hz
    bool pitchWobble = true;
    bool alwaysVoice = false;

    // Tongue
    double tonguePosition = 0.0;   // 0 = back, 1 = front
    double tongueHeight = 0.738;   // 0 = low/open, 1 = high

    // Free constrictions (fingers on the tract in the original)
    struct Constriction
    {
        double position = 0.8;     // 0 = throat, 1 = lips
        double closure = 0.0;      // 0 = open (no effect), 1 = fully closed
        double fricative = 1.0;    // fricative noise amount
    };
    std::array<Constriction, 2> constrictions {};

    // Nasal
    double nasal = 0.0;            // velum opening, 0 = closed

    // Performance / output
    double glideSecondsPerOctave = 0.078;
    double outputGain = 1.0;       // linear
};

class VoiceEngine
{
public:
    static constexpr double internalSampleRate = 48000.0;
    static constexpr int blockSize = 64;
    static constexpr double blockSeconds = blockSize / internalSampleRate;
    static constexpr int numConstrictions = 2;

    VoiceEngine();

    void reset();
    void setParams (const VoiceParams& p) noexcept { params = p; }

    // Note gating. frequency already includes the note's pitch (bend is applied separately).
    void noteOn (double frequencyHz) noexcept;
    void setNoteFrequency (double frequencyHz) noexcept { noteFrequency = frequencyHz; }
    void noteOff() noexcept { noteHeld = false; }
    void setPitchBendSemitones (double semis) noexcept { bendSemitones = semis; }
    bool isNoteHeld() const noexcept { return noteHeld; }

    // Renders numSamples at the internal rate.
    void render (float* out, int numSamples) noexcept;

    // --- Mapping from user controls to the original's units (also used for value text) ---
    static double constrictionIndex (double position) noexcept;
    static double constrictionTouchDiameter (double closure) noexcept;
    static bool constrictionActive (double closure) noexcept { return closure > 1.0e-4; }
    static double tongueDiameterFromHeight (double height) noexcept;
    static double tongueIndexFromPosition (double position, double height) noexcept;
    static double velumFromNasal (double nasal) noexcept { return 0.01 + nasal * (0.4 - 0.01); }

    const Tract& getTract() const noexcept { return tract; }
    const Glottis& getGlottis() const noexcept { return glottis; }

private:
    void renderBlock() noexcept;
    void applyControls() noexcept;

    VoiceParams params;
    Glottis glottis;
    Tract tract;
    WhiteNoise whiteNoise;
    BandpassBiquad aspirateFilter, fricativeFilter;

    std::array<double, numConstrictions> fricativeIntensity {};

    bool noteHeld = false;
    bool hasNotePitch = false;
    double noteFrequency = 140.0;
    double bendSemitones = 0.0;
    double currentGain = 1.0;

    std::array<float, blockSize> block {};
    int blockPos = blockSize;
};

} // namespace purpletuba::dsp
