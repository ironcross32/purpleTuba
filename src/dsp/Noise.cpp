// Purple Tuba - noise sources for the Pink Trombone port.
// Derived from Pink Trombone 1.1 (c) 2017 Neil Thapen, MIT license; see THIRD_PARTY_NOTICES.md.
// The simplex noise is based on public-domain code by Stefan Gustavson, Peter Eastman
// and Joseph Gentle.
#include "dsp/Noise.h"

#include <cmath>

namespace purpletuba::dsp
{

namespace
{
    struct Grad { double x, y; };

    // The 2D part of the 3D gradient set used by the original (z is never read in 2D).
    constexpr Grad grad3[12] = { { 1, 1 }, { -1, 1 }, { 1, -1 }, { -1, -1 },
                                 { 1, 0 }, { -1, 0 }, { 1, 0 }, { -1, 0 },
                                 { 0, 1 }, { 0, -1 }, { 0, 1 }, { 0, -1 } };

    constexpr uint8_t p[256] = {
        151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194, 233, 7,   225, 140, 36,  103, 30,  69,  142,
        8,   99,  37,  240, 21,  10,  23,  190, 6,   148, 247, 120, 234, 75,  0,   26,  197, 62,  94,  252, 219, 203,
        117, 35,  11,  32,  57,  177, 33,  88,  237, 149, 56,  87,  174, 20,  125, 136, 171, 168, 68,  175, 74,  165,
        71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111, 229, 122, 60,  211, 133, 230, 220, 105, 92,  41,
        55,  46,  245, 40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,  209, 76,  132, 187, 208, 89,
        18,  169, 200, 196, 135, 130, 116, 188, 159, 86,  164, 100, 109, 198, 173, 186, 3,   64,  52,  217, 226, 250,
        124, 123, 5,   202, 38,  147, 118, 126, 255, 82,  85,  212, 207, 206, 59,  227, 47,  16,  58,  17,  182, 189,
        28,  42,  223, 183, 170, 213, 119, 248, 152, 2,   44,  154, 163, 70,  221, 153, 101, 155, 167, 43,  172, 9,
        129, 22,  39,  253, 19,  98,  108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,  228, 251, 34,
        242, 193, 238, 210, 144, 12,  191, 179, 162, 241, 81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,
        181, 199, 106, 157, 184, 84,  204, 176, 115, 121, 50,  45,  127, 4,   150, 254, 138, 236, 205, 93,  222, 114,
        67,  29,  24,  72,  243, 141, 128, 195, 78,  66,  215, 61,  156, 180
    };

    const double F2 = 0.5 * (std::sqrt (3.0) - 1.0);
    const double G2 = (3.0 - std::sqrt (3.0)) / 6.0;
}

SimplexNoise::SimplexNoise (uint32_t s) { seed (s); }

void SimplexNoise::seed (uint32_t s)
{
    s &= 0xffffu;
    if (s < 256)
        s |= s << 8;

    for (int i = 0; i < 256; ++i)
    {
        const auto v = static_cast<uint8_t> ((i & 1) ? (p[i] ^ (s & 255)) : (p[i] ^ ((s >> 8) & 255)));
        perm[(size_t) i] = perm[(size_t) i + 256] = v;
        gradIndex[(size_t) i] = gradIndex[(size_t) i + 256] = static_cast<uint8_t> (v % 12);
    }
}

double SimplexNoise::simplex2 (double xin, double yin) const noexcept
{
    const double s = (xin + yin) * F2;
    int i = static_cast<int> (std::floor (xin + s));
    int j = static_cast<int> (std::floor (yin + s));
    const double t = (i + j) * G2;
    const double x0 = xin - i + t;
    const double y0 = yin - j + t;

    int i1, j1;
    if (x0 > y0) { i1 = 1; j1 = 0; }
    else         { i1 = 0; j1 = 1; }

    const double x1 = x0 - i1 + G2;
    const double y1 = y0 - j1 + G2;
    const double x2 = x0 - 1 + 2 * G2;
    const double y2 = y0 - 1 + 2 * G2;

    i &= 255;
    j &= 255;
    const auto& g0 = grad3[gradIndex[(size_t) (i + perm[(size_t) j])]];
    const auto& g1 = grad3[gradIndex[(size_t) (i + i1 + perm[(size_t) (j + j1)])]];
    const auto& g2 = grad3[gradIndex[(size_t) (i + 1 + perm[(size_t) (j + 1)])]];

    auto corner = [] (double tc, const Grad& g, double x, double y)
    {
        if (tc < 0)
            return 0.0;
        tc *= tc;
        return tc * tc * (g.x * x + g.y * y);
    };

    const double n0 = corner (0.5 - x0 * x0 - y0 * y0, g0, x0, y0);
    const double n1 = corner (0.5 - x1 * x1 - y1 * y1, g1, x1, y1);
    const double n2 = corner (0.5 - x2 * x2 - y2 * y2, g2, x2, y2);
    return 70.0 * (n0 + n1 + n2);
}

void BandpassBiquad::setup (double sampleRate, double frequency, double q) noexcept
{
    constexpr double pi = 3.14159265358979323846;
    const double w0 = 2.0 * pi * frequency / sampleRate;
    const double alpha = std::sin (w0) / (2.0 * q);
    const double a0 = 1.0 + alpha;
    b0 = alpha / a0;
    b1 = 0.0;
    b2 = -alpha / a0;
    a1 = -2.0 * std::cos (w0) / a0;
    a2 = (1.0 - alpha) / a0;
}

} // namespace purpletuba::dsp
