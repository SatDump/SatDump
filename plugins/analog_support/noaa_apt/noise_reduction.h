#pragma once

#include "common/dsp/block.h"
#include <fftw3.h>

namespace dsp
{
    class AptNoiseReductionBlock : public Block<complex_t, complex_t>
    {
    private:
        void work();

        complex_t *forwFFTIn;
        complex_t *forwFFTOut;
        complex_t *backFFTIn;
        complex_t *backFFTOut;

        fftwf_plan forwardPlan;
        fftwf_plan backwardPlan;

        complex_t *buffer;
        complex_t *bufferStart;

        float *fftWin;

        float *ampBuf;

        int _bins;

        int process(int count, const complex_t *in, complex_t *out);

    public:
        AptNoiseReductionBlock(std::shared_ptr<dsp::stream<complex_t>> input, int bins);
        ~AptNoiseReductionBlock();
    };
}