#pragma once

#include "block.h"

namespace dsp
{
    class AGCBlock : public Block<complex_t, complex_t>
    {
    private:
        float rate;      // adjustment rate
        float reference; // reference value
        float gain;      // current gain
        float max_gain;  // max allowable gain
        void work();

    public:
        AGCBlock(std::shared_ptr<dsp::stream<complex_t>> input, float agc_rate, float reference, float gain, float max_gain);
    };
}