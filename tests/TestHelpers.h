#pragma once

#include <juce_events/juce_events.h>

#include <cmath>
#include <vector>

namespace testhelpers
{

// The processor's APVTS needs the message manager.
inline void ensureJuceInitialised()
{
    static juce::ScopedJuceInitialiser_GUI init;
}

inline double rms (const std::vector<float>& x, size_t start = 0)
{
    double sum = 0.0;
    size_t n = 0;
    for (size_t i = start; i < x.size(); ++i, ++n)
        sum += (double) x[i] * x[i];
    return n ? std::sqrt (sum / (double) n) : 0.0;
}

inline bool allFinite (const std::vector<float>& x)
{
    for (auto v : x)
        if (! std::isfinite (v))
            return false;
    return true;
}

// Frequency (Hz) of the strongest spectral peak between lo and hi, by direct DFT on a Hann window.
inline double peakFrequency (const std::vector<float>& x, double sampleRate, double lo, double hi, double stepHz = 5.0)
{
    const double pi = 3.14159265358979323846;
    const size_t n = x.size();
    double bestF = lo, bestMag = -1.0;

    for (double f = lo; f <= hi; f += stepHz)
    {
        double re = 0.0, im = 0.0;
        const double w = 2.0 * pi * f / sampleRate;
        for (size_t i = 0; i < n; ++i)
        {
            const double win = 0.5 - 0.5 * std::cos (2.0 * pi * (double) i / (double) (n - 1));
            re += x[i] * win * std::cos (w * (double) i);
            im -= x[i] * win * std::sin (w * (double) i);
        }
        const double mag = re * re + im * im;
        if (mag > bestMag)
        {
            bestMag = mag;
            bestF = f;
        }
    }
    return bestF;
}

} // namespace testhelpers
