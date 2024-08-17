#pragma once

#include "common/dsp/complex.h"
#include "../block.h"

namespace ndsp
{
    template <typename T>
    class Agc : public ndsp::Block
    {
    private:
        void work();

        float rate;      // adjustment rate
        float reference; // reference value
        float gain;      // current gain
        float max_gain;  // max allowable gain

    public:
        float d_rate = -1;
        float d_reference = -1;
        float d_gain = -1;
        float d_max_gain = -1;

    public:
        Agc();
        ~Agc();

        void set_params(nlohmann::json p = {});
        void start();
    };
}