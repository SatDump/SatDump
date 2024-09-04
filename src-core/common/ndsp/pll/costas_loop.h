#pragma once

#include "common/dsp/complex.h"
#include "../block.h"

namespace ndsp
{
    class CostasLoop : public ndsp::Block
    {
    private:
        void work();

        float error = 0;

        float phase = 0, freq = 0;
        float alpha, beta;

        float freq_limit_min, freq_limit_max;

        complex_t tmp_val;

    public:
        int d_order = -1;
        float d_loop_bw = -1;
        float d_freq_limit = -1;

    public:
        CostasLoop();

        void set_params(nlohmann::json p = {});
        void start();

        float getFreq() { return freq; } // TODO
    };
}