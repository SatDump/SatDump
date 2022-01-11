#pragma once

#include "block.h"
#include <vector>

namespace dsp
{
    class CCRationalResamplerBlock : public Block<complex_t, complex_t>
    {
    private:
        // Settings
        int d_interpolation;
        int d_decimation;
        int d_ctr;

        // Buffer
        complex_t *buffer;
        int in_buffer;

        // Taps
        float **taps;
        int nfilt; // Number of filters (one per phase)
        int ntaps;

        void work();

    public:
        CCRationalResamplerBlock(std::shared_ptr<dsp::stream<complex_t>> input, unsigned interpolation, unsigned decimation, std::vector<float> custom_taps = std::vector<float>());
        ~CCRationalResamplerBlock();
    };
}