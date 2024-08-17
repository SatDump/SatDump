#pragma once

#include "common/dsp/complex.h"
#include "../block.h"

namespace ndsp
{
    class PMToBPSK : public ndsp::Block
    {
    private:
        void work();

        complex_t phase_delta, phase;

    public:
        double d_samplerate = -1;
        double d_symbolrate = -1;

    public:
        PMToBPSK();

        void set_params(nlohmann::json p = {});
        void start();
    };
}