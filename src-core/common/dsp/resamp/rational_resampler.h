#pragma once

#include "common/dsp/block.h"
#include <vector>
#include "polyphase_bank.h"

namespace dsp
{
    template <typename T>
    class RationalResamplerBlock : public Block<T, T>
    {
    private:
        // Settings
        int d_interpolation;
        int d_decimation;
        int d_ctr;

        int inc = 0, outc = 0;

        // Buffer
        T *buffer;

        // Taps
        PolyphaseBank pfb;

        void work();

    public:
        RationalResamplerBlock(std::shared_ptr<dsp::stream<T>> input, unsigned interpolation, unsigned decimation, std::vector<float> custom_taps = std::vector<float>());
        ~RationalResamplerBlock();

        int process(T *input, int nsamples, T *output);
    };
}