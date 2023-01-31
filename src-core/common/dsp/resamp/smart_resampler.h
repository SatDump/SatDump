#pragma once

#include "common/dsp/block.h"
#include <vector>
#include "power_decim.h"
#include "rational_resampler.h"

namespace dsp
{
    template <typename T>
    class SmartResamplerBlock : public Block<T, T>
    {
    private:
        int d_interpolation;
        int d_decimation;

        bool use_decim;
        bool use_resamp;

        std::unique_ptr<PowerDecimatorBlock<T>> pdecim;
        std::unique_ptr<RationalResamplerBlock<T>> rresamp;

        void work();

    public:
        SmartResamplerBlock(std::shared_ptr<dsp::stream<T>> input, unsigned interpolation, unsigned decimation);
        ~SmartResamplerBlock();

        int process(T *input, int nsamples, T *output);
    };
}