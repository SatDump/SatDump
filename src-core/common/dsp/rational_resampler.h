#pragma once

#include "block.h"
#include <vector>

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

        // Buffer
        T *buffer;
        int in_buffer;

        // Taps
        float **taps;
        int nfilt; // Number of filters (one per phase)
        int ntaps;

        void work();

    public:
        RationalResamplerBlock(std::shared_ptr<dsp::stream<T>> input, unsigned interpolation, unsigned decimation, std::vector<float> custom_taps = std::vector<float>());
        ~RationalResamplerBlock();
    };
}