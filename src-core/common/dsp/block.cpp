#include "block.h"

namespace dsp
{
    float branchless_clip(float x, float clip)
    {
        return 0.5 * (std::abs(x + clip) - std::abs(x - clip));
    }

    double hz_to_rad(double freq, double samplerate)
    {
        return 2.0 * M_PI * (freq / samplerate);
    }

    double rad_to_hz(double rad, double samplerate)
    {
        return (rad / (2.0 * M_PI)) * samplerate;
    }
};