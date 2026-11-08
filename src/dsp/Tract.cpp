// Purple Tuba - vocal tract waveguide.
// Port of the Tract and TractUI shaping code from Pink Trombone 1.1 (c) 2017 Neil Thapen,
// MIT license; see THIRD_PARTY_NOTICES.md.
#include "dsp/Tract.h"

#include <algorithm>
#include <cmath>

namespace purpletuba::dsp
{

namespace
{
    constexpr double pi = 3.14159265358979323846;
    constexpr double gridOffset = 1.7;

    double moveTowards (double current, double target, double amountUp, double amountDown) noexcept
    {
        if (current < target)
            return std::min (current + amountUp, target);
        return std::max (current - amountDown, target);
    }
}

void Tract::prepare (double sr)
{
    sampleRate = sr;
    reset();
}

void Tract::reset()
{
    for (int i = 0; i < n; ++i)
    {
        double d;
        if (i < 7.0 * n / 44.0 - 0.5) d = 0.6;
        else if (i < 12.0 * n / 44.0) d = 1.1;
        else d = 1.5;
        diameter[(size_t) i] = restDiameter[(size_t) i] = targetDiameter[(size_t) i] = d;
    }

    R.fill (0); L.fill (0); A.fill (0);
    reflection.fill (0); newReflection.fill (0);
    junctionOutputR.fill (0); junctionOutputL.fill (0);
    noseR.fill (0); noseL.fill (0); noseA.fill (0);
    noseJunctionOutputR.fill (0); noseJunctionOutputL.fill (0); noseReflection.fill (0);

    for (int i = 0; i < noseLength; ++i)
    {
        const double d = 2.0 * (static_cast<double> (i) / noseLength);
        double dia = d < 1.0 ? 0.4 + 1.6 * d : 0.5 + 1.5 * (2.0 - d);
        noseDiameter[(size_t) i] = std::min (dia, 1.9);
    }

    reflectionLeft = reflectionRight = reflectionNose = 0;
    newReflectionLeft = newReflectionRight = newReflectionNose = 0;
    lastObstruction = -1;
    lipOutput = noseOutput = 0;
    numTransients = 0;
    numTurbulence = 0;
    velumTarget = 0.01;

    calculateReflections();
    calculateNoseReflections();
    noseDiameter[0] = velumTarget;
    noseA[0] = velumTarget * velumTarget;

    // Default tongue from the original TractUI, then settle the shape immediately.
    setTongue (12.9, 2.43);
    resetTargetToRest();
    diameter = targetDiameter;
    calculateReflections();
    calculateReflections(); // twice so reflection == newReflection (no initial interpolation jump)
}

void Tract::tongueIndexRange (double tongueDiameter, double& lowIndex, double& highIndex) noexcept
{
    double fromPoint = (outerTongueControlRadius - tongueDiameter) / (outerTongueControlRadius - innerTongueControlRadius);
    fromPoint = std::clamp (fromPoint, 0.0, 1.0);
    fromPoint = std::pow (fromPoint, 0.58) - 0.2 * (fromPoint * fromPoint - fromPoint);
    const double out = fromPoint * 0.5 * (tongueUpperIndexBound - tongueLowerIndexBound);
    lowIndex = tongueIndexCentre - out;
    highIndex = tongueIndexCentre + out;
}

void Tract::setTongue (double tongueIndex, double tongueDiameter) noexcept
{
    for (int i = bladeStart; i < lipStart; ++i)
    {
        const double t = 1.1 * pi * (tongueIndex - i) / (tipStart - bladeStart);
        const double fixedTongueDiameter = 2.0 + (tongueDiameter - 2.0) / 1.5;
        double curve = (1.5 - fixedTongueDiameter + gridOffset) * std::cos (t);
        if (i == bladeStart - 2 || i == lipStart - 1) curve *= 0.8;
        if (i == bladeStart || i == lipStart - 2) curve *= 0.94;
        restDiameter[(size_t) i] = 1.5 - curve;
    }
}

void Tract::resetTargetToRest() noexcept
{
    targetDiameter = restDiameter;
}

void Tract::applyConstriction (double index, double touchDiameter) noexcept
{
    double d = touchDiameter - 0.3;
    if (d < 0) d = 0;

    double width;
    if (index < 25) width = 10;
    else if (index >= tipStart) width = 5;
    else width = 10 - 5 * (index - 25) / (tipStart - 25);

    if (! (index >= 2 && index < n && d < 3))
        return;

    const int intIndex = static_cast<int> (std::round (index));
    for (int i = -static_cast<int> (std::ceil (width)) - 1; i < width + 1; ++i)
    {
        const int k = intIndex + i;
        if (k < 0 || k >= n)
            continue;

        double relpos = k - index;
        relpos = std::abs (relpos) - 0.5;

        double shrink;
        if (relpos <= 0) shrink = 0;
        else if (relpos > width) shrink = 1;
        else shrink = 0.5 * (1 - std::cos (pi * relpos / width));

        auto& target = targetDiameter[(size_t) k];
        if (d < target)
            target = d + (target - d) * shrink;
    }
}

void Tract::setTurbulenceSources (const TurbulenceSource* sources, int count) noexcept
{
    numTurbulence = std::clamp (count, 0, maxTurbulenceSources);
    for (int i = 0; i < numTurbulence; ++i)
        turbulence[(size_t) i] = sources[i];
}

void Tract::reshapeTract (double deltaTime) noexcept
{
    double amount = deltaTime * movementSpeed;
    int newLastObstruction = -1;

    for (int i = 0; i < n; ++i)
    {
        const double d = diameter[(size_t) i];
        const double target = targetDiameter[(size_t) i];
        if (d <= 0) newLastObstruction = i;

        double slowReturn;
        if (i < noseStart) slowReturn = 0.6;
        else if (i >= tipStart) slowReturn = 1.0;
        else slowReturn = 0.6 + 0.4 * (i - noseStart) / static_cast<double> (tipStart - noseStart);

        diameter[(size_t) i] = moveTowards (d, target, slowReturn * amount, 2 * amount);
    }

    if (lastObstruction > -1 && newLastObstruction == -1 && noseA[0] < 0.05)
        addTransient (lastObstruction);
    lastObstruction = newLastObstruction;

    amount = deltaTime * movementSpeed;
    noseDiameter[0] = moveTowards (noseDiameter[0], velumTarget, amount * 0.25, amount * 0.1);
    noseA[0] = noseDiameter[0] * noseDiameter[0];
}

void Tract::calculateReflections() noexcept
{
    for (int i = 0; i < n; ++i)
        A[(size_t) i] = diameter[(size_t) i] * diameter[(size_t) i]; // ignoring pi etc.

    for (int i = 1; i < n; ++i)
    {
        reflection[(size_t) i] = newReflection[(size_t) i];
        if (A[(size_t) i] == 0)
            newReflection[(size_t) i] = 0.999; // to prevent some bad behaviour if 0
        else
            newReflection[(size_t) i] = (A[(size_t) i - 1] - A[(size_t) i]) / (A[(size_t) i - 1] + A[(size_t) i]);
    }

    // junction with the nose
    reflectionLeft = newReflectionLeft;
    reflectionRight = newReflectionRight;
    reflectionNose = newReflectionNose;
    const double sum = A[noseStart] + A[noseStart + 1] + noseA[0];
    newReflectionLeft = (2 * A[noseStart] - sum) / sum;
    newReflectionRight = (2 * A[noseStart + 1] - sum) / sum;
    newReflectionNose = (2 * noseA[0] - sum) / sum;
}

void Tract::calculateNoseReflections() noexcept
{
    for (int i = 0; i < noseLength; ++i)
        noseA[(size_t) i] = noseDiameter[(size_t) i] * noseDiameter[(size_t) i];

    for (int i = 1; i < noseLength; ++i)
        noseReflection[(size_t) i] = (noseA[(size_t) i - 1] - noseA[(size_t) i]) / (noseA[(size_t) i - 1] + noseA[(size_t) i]);
}

void Tract::runStep (double glottalOutput, double turbulenceNoise, double noiseModulator, double lambda) noexcept
{
    // mouth
    processTransients();
    addTurbulenceNoise (turbulenceNoise, noiseModulator);

    junctionOutputR[0] = L[0] * glottalReflection + glottalOutput;
    junctionOutputL[n] = R[n - 1] * lipReflection;

    for (int i = 1; i < n; ++i)
    {
        const double r = reflection[(size_t) i] * (1 - lambda) + newReflection[(size_t) i] * lambda;
        const double w = r * (R[(size_t) i - 1] + L[(size_t) i]);
        junctionOutputR[(size_t) i] = R[(size_t) i - 1] - w;
        junctionOutputL[(size_t) i] = L[(size_t) i] + w;
    }

    // junction with the nose (the interpolation direction matches the original)
    {
        const int i = noseStart;
        double r = newReflectionLeft * (1 - lambda) + reflectionLeft * lambda;
        junctionOutputL[i] = r * R[i - 1] + (1 + r) * (noseL[0] + L[i]);
        r = newReflectionRight * (1 - lambda) + reflectionRight * lambda;
        junctionOutputR[i] = r * L[i] + (1 + r) * (R[i - 1] + noseL[0]);
        r = newReflectionNose * (1 - lambda) + reflectionNose * lambda;
        noseJunctionOutputR[0] = r * noseL[0] + (1 + r) * (L[i] + R[i - 1]);
    }

    for (int i = 0; i < n; ++i)
    {
        R[(size_t) i] = junctionOutputR[(size_t) i] * 0.999;
        L[(size_t) i] = junctionOutputL[(size_t) i + 1] * 0.999;
    }

    lipOutput = R[n - 1];

    // nose
    noseJunctionOutputL[noseLength] = noseR[noseLength - 1] * lipReflection;

    for (int i = 1; i < noseLength; ++i)
    {
        const double w = noseReflection[(size_t) i] * (noseR[(size_t) i - 1] + noseL[(size_t) i]);
        noseJunctionOutputR[(size_t) i] = noseR[(size_t) i - 1] - w;
        noseJunctionOutputL[(size_t) i] = noseL[(size_t) i] + w;
    }

    for (int i = 0; i < noseLength; ++i)
    {
        noseR[(size_t) i] = noseJunctionOutputR[(size_t) i] * fade;
        noseL[(size_t) i] = noseJunctionOutputL[(size_t) i + 1] * fade;
    }

    noseOutput = noseR[noseLength - 1];
}

void Tract::finishBlock (double blockSeconds) noexcept
{
    reshapeTract (blockSeconds);
    calculateReflections();
}

void Tract::addTransient (int position) noexcept
{
    if (numTransients >= maxTransients)
        return;
    transients[(size_t) numTransients++] = { position, 0.0, 0.2, 0.3, 200.0 };
}

void Tract::processTransients() noexcept
{
    for (int i = 0; i < numTransients; ++i)
    {
        auto& trans = transients[(size_t) i];
        const double amplitude = trans.strength * std::pow (2.0, -trans.exponent * trans.timeAlive);
        R[(size_t) trans.position] += amplitude / 2;
        L[(size_t) trans.position] += amplitude / 2;
        trans.timeAlive += 1.0 / (sampleRate * 2);
    }

    for (int i = numTransients - 1; i >= 0; --i)
    {
        if (transients[(size_t) i].timeAlive > transients[(size_t) i].lifeTime)
        {
            transients[(size_t) i] = transients[(size_t) numTransients - 1];
            --numTransients;
        }
    }
}

void Tract::addTurbulenceNoise (double turbulenceNoise, double noiseModulator) noexcept
{
    for (int j = 0; j < numTurbulence; ++j)
    {
        const auto& t = turbulence[(size_t) j];
        if (t.index < 2 || t.index > n) continue;
        if (t.diameter <= 0) continue;
        if (t.intensity == 0) continue;
        addTurbulenceNoiseAtIndex (0.66 * turbulenceNoise * t.intensity * noiseModulator, t.index, t.diameter);
    }
}

void Tract::addTurbulenceNoiseAtIndex (double turbulenceNoise, double index, double d) noexcept
{
    const int i = static_cast<int> (std::floor (index));
    const double delta = index - i;
    const double thinness0 = std::clamp (8 * (0.7 - d), 0.0, 1.0);
    const double openness = std::clamp (30 * (d - 0.3), 0.0, 1.0);
    const double noise0 = turbulenceNoise * (1 - delta) * thinness0 * openness;
    const double noise1 = turbulenceNoise * delta * thinness0 * openness;

    // The original writes past the end of its typed arrays near the lips (silently ignored in JS).
    if (i + 1 < n) { R[(size_t) i + 1] += noise0 / 2; L[(size_t) i + 1] += noise0 / 2; }
    if (i + 2 < n) { R[(size_t) i + 2] += noise1 / 2; L[(size_t) i + 2] += noise1 / 2; }
}

} // namespace purpletuba::dsp
