#pragma once

#include "common/dsp/complex.h"
#include "../block.h"

#include "common/dsp/resamp/polyphase_bank.h"

namespace ndsp
{
    template <typename T>
    class RationalResampler : public ndsp::Block
    {
    private:
        void work();

        // Settings
        int interpolation;
        int decimation;
        int d_ctr;

        int inc = 0, outc = 0;

        // Buffer
        T *buffer = nullptr;

        // Taps
        dsp::PolyphaseBank pfb;

    public:
        int d_interpolation = -1;
        int d_decimation = -1;

    public:
        RationalResampler();
        ~RationalResampler();

        int process(T *input, int nsamples, T *output);
        void set_ratio(unsigned interpolation, unsigned decimation, std::vector<float> custom_taps = std::vector<float>());

        void set_params(nlohmann::json p = {});
        void start();
    };
}