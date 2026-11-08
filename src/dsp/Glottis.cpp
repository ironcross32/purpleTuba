// Purple Tuba - glottal source.
// Port of the Glottis object from Pink Trombone 1.1 (c) 2017 Neil Thapen, MIT license;
// see THIRD_PARTY_NOTICES.md.
#include "dsp/Glottis.h"

#include <algorithm>
#include <cmath>

namespace purpletuba::dsp
{

namespace
{
    constexpr double pi = 3.14159265358979323846;

    // The original changes intensity by +0.13 / -0.05 per 512-sample block at 48 kHz.
    constexpr double originalBlockSeconds = 512.0 / 48000.0;
    constexpr double attackPerSecond = 0.13 / originalBlockSeconds;
    constexpr double releasePerSecond = 0.05 / originalBlockSeconds;
}

void Glottis::prepare (double sr)
{
    sampleRate = sr;
    reset();
}

void Glottis::reset()
{
    timeInWaveform = 0.0;
    totalTime = 0.0;
    smoothFrequency = oldFrequency = newFrequency = controls.frequency;
    oldTenseness = newTenseness = controls.tenseness;
    intensity = 0.0;
    setupWaveform (0.0);
}

void Glottis::setControls (const Controls& c) noexcept
{
    controls = c;
    controls.tenseness = std::clamp (controls.tenseness, 0.0, 1.0);
    loudnessGain = std::pow (controls.tenseness, 0.25) * controls.loudness;
}

void Glottis::snapPitchIfSilent() noexcept
{
    if (intensity <= 0.0)
        smoothFrequency = oldFrequency = newFrequency = controls.frequency;
}

double Glottis::runStep (double lambda, double aspirationNoise) noexcept
{
    const double timeStep = 1.0 / sampleRate;
    timeInWaveform += timeStep;
    totalTime += timeStep;

    if (timeInWaveform > waveformLength)
    {
        timeInWaveform -= waveformLength;
        setupWaveform (lambda);
    }

    double out = normalizedLFWaveform (timeInWaveform / waveformLength);

    double aspiration = intensity * (1.0 - std::sqrt (controls.tenseness)) * getNoiseModulator() * aspirationNoise;
    aspiration *= 0.2 + 0.02 * noise.simplex1 (totalTime * 1.99);
    out += aspiration * controls.breathiness;
    return out;
}

double Glottis::getNoiseModulator() const noexcept
{
    const double voiced = 0.1 + 0.2 * std::max (0.0, std::sin (pi * 2.0 * timeInWaveform / waveformLength));
    const double ti = controls.tenseness * intensity;
    return ti * voiced + (1.0 - ti) * 0.3;
}

void Glottis::finishBlock (double blockSeconds) noexcept
{
    double vibrato = 0.0;
    vibrato += controls.vibratoAmount * std::sin (2.0 * pi * totalTime * controls.vibratoFrequency);
    vibrato += 0.02 * noise.simplex1 (totalTime * 4.07);
    vibrato += 0.04 * noise.simplex1 (totalTime * 2.15);

    if (controls.pitchWobble)
    {
        vibrato += 0.2 * noise.simplex1 (totalTime * 0.98);
        vibrato += 0.4 * noise.simplex1 (totalTime * 0.5);
    }

    const double target = controls.frequency;
    if (controls.glideSecondsPerOctave <= 0.0)
    {
        smoothFrequency = target;
    }
    else
    {
        const double maxRatio = std::pow (2.0, blockSeconds / controls.glideSecondsPerOctave);
        if (target > smoothFrequency)
            smoothFrequency = std::min (smoothFrequency * maxRatio, target);
        else if (target < smoothFrequency)
            smoothFrequency = std::max (smoothFrequency / maxRatio, target);
    }

    oldFrequency = newFrequency;
    newFrequency = smoothFrequency * (1.0 + vibrato);

    oldTenseness = newTenseness;
    newTenseness = controls.tenseness
                 + 0.1 * noise.simplex1 (totalTime * 0.46)
                 + 0.05 * noise.simplex1 (totalTime * 0.36);

    if (! controls.voiced && controls.alwaysVoice)
        newTenseness += (3.0 - controls.tenseness) * (1.0 - intensity);

    if (controls.voiced || controls.alwaysVoice)
        intensity += attackPerSecond * blockSeconds;
    else
        intensity -= releasePerSecond * blockSeconds;

    intensity = std::clamp (intensity, 0.0, 1.0);
}

void Glottis::setupWaveform (double lambda) noexcept
{
    frequency = oldFrequency * (1.0 - lambda) + newFrequency * lambda;
    frequency = std::max (frequency, 1.0);
    const double tenseness = oldTenseness * (1.0 - lambda) + newTenseness * lambda;
    waveformLength = 1.0 / frequency;

    const double Rd = std::clamp (3.0 * (1.0 - tenseness), 0.5, 2.7);

    // normalized to time = 1, Ee = 1
    const double Ra = -0.01 + 0.048 * Rd;
    const double Rk = 0.224 + 0.118 * Rd;
    const double Rg = (Rk / 4.0) * (0.5 + 1.2 * Rk) / (0.11 * Rd - Ra * (0.5 + 1.2 * Rk));

    const double Ta = Ra;
    const double Tp = 1.0 / (2.0 * Rg);
    const double te = Tp + Tp * Rk;

    const double eps = 1.0 / Ta;
    const double sh = std::exp (-eps * (1.0 - te));
    const double delta = 1.0 - sh;

    double rhsIntegral = (1.0 / eps) * (sh - 1.0) + (1.0 - te) * sh;
    rhsIntegral = rhsIntegral / delta;

    const double totalLowerIntegral = -(te - Tp) / 2.0 + rhsIntegral;
    const double totalUpperIntegral = -totalLowerIntegral;

    const double om = pi / Tp;
    const double s = std::sin (om * te);
    const double y = -pi * s * totalUpperIntegral / (Tp * 2.0);
    const double z = std::log (y);
    const double a = z / (Tp / 2.0 - te);
    const double e0 = -1.0 / (s * std::exp (a * te));

    alpha = a;
    E0 = e0;
    epsilon = eps;
    shift = sh;
    Delta = delta;
    Te = te;
    omega = om;
}

double Glottis::normalizedLFWaveform (double t) const noexcept
{
    double output;
    if (t > Te)
        output = (-std::exp (-epsilon * (t - Te)) + shift) / Delta;
    else
        output = E0 * std::exp (alpha * t) * std::sin (omega * t);

    return output * intensity * loudnessGain;
}

} // namespace purpletuba::dsp
