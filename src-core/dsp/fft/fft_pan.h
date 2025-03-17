#pragma once

#include "dsp/block.h"
#include "common/dsp/complex.h"
#include <fftw3.h>
#include <volk/volk_alloc.hh>
#include <functional>
#include <mutex>

namespace satdump
{
    namespace ndsp
    {
        class FFTPanBlock : public Block
        {
        private:
            std::mutex fft_mutex;
            volk::vector<float> fft_taps;
            int fft_size;
            bool work();

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
            FFTPanBlock();
            ~FFTPanBlock();
            void set_fft_settings(int size, uint64_t samplerate, int rate = 60);

            std::function<void(float *)> on_fft = [](float *) {};

            float *output_fft_buff;

            float avg_num = 10;

            nlohmann::json get_cfg()
            {
                nlohmann::json v;
                //    v["rate"] = rate;
                return v;
            }

            void set_cfg(nlohmann::json /*v*/)
            {
                //  rate = v["rate"];
            }
        };
    }
}