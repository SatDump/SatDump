#pragma once

#include "common/dsp/block.h"
#include <vector>
#include "common/dsp/filter/decimating_fir.h"

namespace dsp
{
    template <typename T>
    class PowerDecimatorBlock : public Block<T, T>
    {
    private:
        int d_decimation;

        std::vector<std::unique_ptr<DecimatingFIRBlock<T>>> fir_stages;

        void work();

    public:
        PowerDecimatorBlock(std::shared_ptr<dsp::stream<T>> input, unsigned decimation);
        ~PowerDecimatorBlock();

        int process(T *input, int nsamples, T *output);

        static int max_decim();
    };
}