// Purple Tuba - vocal tract waveguide (Kelly-Lochbaum, 44 sections, with nasal branch).
// Port of the Tract and TractUI shaping code from Pink Trombone 1.1 (c) 2017 Neil Thapen,
// MIT license; see THIRD_PARTY_NOTICES.md.
#pragma once

#include <array>

namespace purpletuba::dsp
{

class Tract
{
public:
    static constexpr int n = 44;
    static constexpr int bladeStart = 10;
    static constexpr int tipStart = 32;
    static constexpr int lipStart = 39;
    static constexpr int noseLength = 28;
    static constexpr int noseStart = n - noseLength + 1; // 17

    // Tongue control limits from the original TractUI.
    static constexpr double innerTongueControlRadius = 2.05;
    static constexpr double outerTongueControlRadius = 3.5;
    static constexpr double tongueLowerIndexBound = bladeStart + 2;
    static constexpr double tongueUpperIndexBound = tipStart - 3;
    static constexpr double tongueIndexCentre = 0.5 * (tongueLowerIndexBound + tongueUpperIndexBound);

    // Lowest and highest index a constriction (a "finger" in the original) can sit at.
    static constexpr double minConstrictionIndex = 2.0;
    static constexpr double maxConstrictionIndex = n - 1.0;

    static constexpr int maxTurbulenceSources = 4;
    static constexpr int maxTransients = 16;

    struct TurbulenceSource
    {
        double index = 0;          // position along the tract
        double diameter = 0;       // the original's raw touch diameter (before the -0.3 offset)
        double intensity = 0;      // fricative intensity 0..1
    };

    void prepare (double sampleRate);
    void reset();

    // --- Shape control, call before each control block ---------------------------------
    // Rebuilds the rest shape from the tongue. index and diameter are in the original's units.
    void setTongue (double tongueIndex, double tongueDiameter) noexcept;
    // Resets the target shape to the rest shape (call after setTongue, before constrictions).
    void resetTargetToRest() noexcept;
    // Narrows the target shape around index; touchDiameter is the original's raw touch diameter.
    void applyConstriction (double index, double touchDiameter) noexcept;
    void setVelumTarget (double v) noexcept { velumTarget = v; }
    void setTurbulenceSources (const TurbulenceSource* sources, int count) noexcept;

    // --- Audio --------------------------------------------------------------------------
    // One half-step (the tract runs at twice the sample rate).
    void runStep (double glottalOutput, double turbulenceNoise, double noiseModulator, double lambda) noexcept;
    void finishBlock (double blockSeconds) noexcept;

    double getLipOutput() const noexcept { return lipOutput; }
    double getNoseOutput() const noexcept { return noseOutput; }

    // Exposed for tests / inspection.
    const std::array<double, n>& getDiameters() const noexcept { return diameter; }
    const std::array<double, n>& getTargetDiameters() const noexcept { return targetDiameter; }
    double getVelumDiameter() const noexcept { return noseDiameter[0]; }

    // The original TractUI's mapping from tongue diameter to the allowed tongue index range.
    static void tongueIndexRange (double tongueDiameter, double& lowIndex, double& highIndex) noexcept;

private:
    void reshapeTract (double deltaTime) noexcept;
    void calculateReflections() noexcept;
    void calculateNoseReflections() noexcept;
    void addTransient (int position) noexcept;
    void processTransients() noexcept;
    void addTurbulenceNoise (double turbulenceNoise, double noiseModulator) noexcept;
    void addTurbulenceNoiseAtIndex (double turbulenceNoise, double index, double diameter) noexcept;

    double sampleRate = 48000.0;

    std::array<double, n> R {}, L {}, diameter {}, restDiameter {}, targetDiameter {}, A {};
    std::array<double, n + 1> reflection {}, newReflection {}, junctionOutputR {}, junctionOutputL {};

    std::array<double, noseLength> noseR {}, noseL {}, noseDiameter {}, noseA {};
    std::array<double, noseLength + 1> noseJunctionOutputR {}, noseJunctionOutputL {}, noseReflection {};

    double reflectionLeft = 0, reflectionRight = 0, reflectionNose = 0;
    double newReflectionLeft = 0, newReflectionRight = 0, newReflectionNose = 0;

    static constexpr double glottalReflection = 0.75;
    static constexpr double lipReflection = -0.85;
    static constexpr double fade = 1.0;
    static constexpr double movementSpeed = 15.0; // cm per second

    int lastObstruction = -1;
    double velumTarget = 0.01;
    double lipOutput = 0, noseOutput = 0;

    struct Transient { int position; double timeAlive; double lifeTime; double strength; double exponent; };
    std::array<Transient, maxTransients> transients {};
    int numTransients = 0;

    std::array<TurbulenceSource, maxTurbulenceSources> turbulence {};
    int numTurbulence = 0;
};

} // namespace purpletuba::dsp
