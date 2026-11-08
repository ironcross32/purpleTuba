// Purple Tuba - noise sources for the Pink Trombone port.
// Derived from Pink Trombone 1.1 (c) 2017 Neil Thapen, MIT license; see THIRD_PARTY_NOTICES.md.
// The simplex noise is based on public-domain code by Stefan Gustavson, Peter Eastman
// and Joseph Gentle.
#pragma once

#include <array>
#include <cstdint>

namespace purpletuba::dsp
{

// 1D simplex noise as used by Pink Trombone (a slice through 2D simplex noise).
class SimplexNoise
{
public:
    explicit SimplexNoise (uint32_t seed = 0x5eed1234u);
    void seed (uint32_t seed);

    double simplex2 (double xin, double yin) const noexcept;
    double simplex1 (double x) const noexcept { return simplex2 (x * 1.2, -x * 0.7); }

private:
    std::array<uint8_t, 512> perm {};
    std::array<uint8_t, 512> gradIndex {};
};

// Uniform white noise in [0, 1), the same distribution as Pink Trombone's Math.random() buffer.
class WhiteNoise
{
public:
    explicit WhiteNoise (uint64_t seed = 0x9e3779b97f4a7c15ull) : state (seed ? seed : 1) {}

    float next() noexcept
    {
        // xorshift64*
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        const auto r = state * 0x2545F4914F6CDD1Dull;
        return static_cast<float> ((r >> 40) * (1.0 / 16777216.0));
    }

private:
    uint64_t state;
};

// Constant 0 dB peak-gain bandpass biquad, matching the Web Audio BiquadFilterNode "bandpass" type.
class BandpassBiquad
{
public:
    void setup (double sampleRate, double frequency, double q) noexcept;
    void reset() noexcept { x1 = x2 = y1 = y2 = 0.0; }

    float process (float in) noexcept
    {
        const double y = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = in;
        y2 = y1; y1 = y;
        return static_cast<float> (y);
    }

private:
    double b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
};

} // namespace purpletuba::dsp
