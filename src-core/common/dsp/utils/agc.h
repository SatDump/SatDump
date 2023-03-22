#pragma once

#include "common/dsp/block.h"

namespace dsp
{
    template <typename T>
    class AGCBlock : public Block<T, T>
    {
    private:
        float rate;      // adjustment rate
        float reference; // reference value
        float gain;      // current gain
        float max_gain;  // max allowable gain
        void work();

    public:
        AGCBlock(std::shared_ptr<dsp::stream<T>> input, float agc_rate, float reference, float gain, float max_gain);
    };
}