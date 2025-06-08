#pragma once

#include <fftw3.h>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class FFTHelper
        {
        public:
            fftwf_plan fft;
            T *fft_in;
            T *fft_out;

        public:
            FFTHelper(int fft_size, bool forward, unsigned int flags = FFTW_ESTIMATE)
            {
                fft_in = (T *)fftwf_malloc(sizeof(T) * fft_size);
                fft_out = (T *)fftwf_malloc(sizeof(T) * fft_size);
                fft = fftwf_plan_dft_1d(fft_size, (fftwf_complex *)fft_in, (fftwf_complex *)fft_out, forward ? FFTW_FORWARD : FFTW_BACKWARD, flags);
            }

            ~FFTHelper()
            {
                fftwf_free(fft);
                fftwf_free(fft_in);
                fftwf_free(fft_out);
            }

            inline void execute() { fftwf_execute(fft); }
        };
    } // namespace ndsp
} // namespace satdump