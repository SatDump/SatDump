#pragma once

#include "common/dsp/complex.h"
#include "../block.h"

#include <mutex>

namespace ndsp
{
    class FreqShift : public ndsp::Block
    {
    private:
        void work();

        std::mutex mtx;
        complex_t phase_delta;
        complex_t phase;

    public:
        double d_samplerate = -1;
        double d_shift = 0;

    public:
        FreqShift();

        void set_params(nlohmann::json p = {});
        void start();

        void set_freq(double samplerate, double freq);
        void set_freq_raw(double freq); // Allows using this as a manual frequency correction block, eg, as a pre-PLL tool
    };
}