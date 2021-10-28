#include "block.h"

namespace dsp
{
    float branchless_clip(float x, float clip)
    {
        return 0.5 * (std::abs(x + clip) - std::abs(x - clip));
    }
};