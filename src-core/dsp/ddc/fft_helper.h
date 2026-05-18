#pragma once

#include <fftw3.h>

namespace satdump
{
    namespace ndsp
    {
        class FFTHelper
        {
        public:
            const int size = 0;
            fftwf_complex *fft_in = nullptr, *fft_out = nullptr;

        private:
            fftwf_plan fft;

        public:
            FFTHelper(int size, bool fwd = true) : size(size)
            {
                fft_in = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * size);
                fft_out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * size);

                fft = fftwf_plan_dft_1d(size, fft_in, fft_out, fwd ? FFTW_FORWARD : FFTW_BACKWARD, FFTW_ESTIMATE);
            }

            inline void execute() { fftwf_execute(fft); }

            ~FFTHelper()
            {
                fftwf_free(fft_in);
                fftwf_free(fft_out);
            }
        };
    } // namespace ndsp
} // namespace satdump