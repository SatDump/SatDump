#pragma once

#include <fftw3.h>
#include "common/dsp/block.h"
#include <vector>

/*
Basic FFT filters implementations.
*/
namespace dsp
{
    template <typename T>
    class FFTFilterBlock : public Block<T, T>
    {
    private:
        T *buffer;
        T *tail;
        T *ffttaps_buffer;
        void work();

        int in_buffer = 0;

        int d_ntaps = 0;
        int d_fftsize = 0;
        int d_nsamples = 0;

        fftwf_complex *fft_fwd_in, *fft_fwd_out;
        fftwf_plan fft_fwd;
        fftwf_complex *fft_inv_in, *fft_inv_out;
        fftwf_plan fft_inv;

    public:
        FFTFilterBlock(std::shared_ptr<dsp::stream<T>> input, std::vector<float> taps);
        ~FFTFilterBlock();
    };
}