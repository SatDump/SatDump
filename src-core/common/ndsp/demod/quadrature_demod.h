#pragma once

#include "common/dsp/complex.h"
#include "../block.h"

namespace ndsp
{
    class QuadratureDemod : public ndsp::Block
    {
    private:
        void work();

        float gain;
        float phase = 0;

    public:
        double d_gain = -1;

    public:
        QuadratureDemod();

        int process(complex_t *input, int nsamples, float *output);

        void set_params(nlohmann::json p = {});
        void start();
    };
}