#pragma once

#include "common/dsp/complex.h"
#include "../block.h"

namespace ndsp
{
    class PLLCarrierTracking : public ndsp::Block
    {
    private:
        void work();

        float max_freq, min_freq;
        float damping, loop_bw;
        float alpha, beta;
        float phase, freq;

    public:
        float d_loop_bw;
        float d_max_freq;
        float d_min_freq;

    public:
        PLLCarrierTracking();

        void set_params(nlohmann::json p = {});
        void start();

        float getFreq() { return freq; }
    };
}