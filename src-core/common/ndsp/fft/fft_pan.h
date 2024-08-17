#pragma once

#include "common/dsp/complex.h"
#include "../block.h"

#include <fftw3.h>
#include <volk/volk_alloc.hh>
#include <functional>
#include <mutex>

namespace ndsp
{
    class FFTPan : public ndsp::Block
    {
    private:
        void work();

        std::mutex fft_mutex;
        volk::vector<float> fft_taps;
        int fft_size;

        void destroy_fft();

        complex_t *fft_input_buffer;
        float *fft_output_buffer = nullptr;

        int in_reshape_buffer = 0;
        complex_t *fft_reshape_buffer;

        fftwf_complex *fftw_in;
        fftwf_complex *fftw_out;
        fftwf_plan fftw_plan;

        int rbuffer_rate = 0;
        int rbuffer_size = 0;
        int rbuffer_skip = 0;

        int reshape_buffer_size = 0;

    public:
        int d_size = 8192;
        uint64_t d_samplerate = -1;
        int d_rate = 30;

    public:
        FFTPan();
        ~FFTPan();

        void set_params(nlohmann::json p = {});
        void start();

        void set_fft_settings(int size, uint64_t samplerate, int rate = 60);

        std::function<void(float *)> on_fft = [](float *) {};

        float avg_num = 10;

        float *d_output_buffer; // TODO FIX
    };
}