#pragma once

#include "common/dsp/block.h"

namespace dsp
{
    template <typename T>
    class AGC2Block : public Block<T, T>
    {
    private:
        const float d_target; // Target magnitude
        const float d_bias;   // Moving bias, to cancel DC offset
        const float d_gain;   // Current gain

        float bias;       // DC Bias
        float moving_avg; // Moving average, used for gain computations
        float gain;       // Current gain

        void work();

    public:
        AGC2Block(std::shared_ptr<dsp::stream<T>> input, float target, float bias, float gain);
    };
}