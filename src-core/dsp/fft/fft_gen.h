#pragma once

#include "common/dsp/block.h"
#include "common/dsp/complex.h"
#include "common/dsp/window/window.h"
#include "core/exception.h"
#include "dsp/block.h"
#include <cstdint>
#include <fftw3.h>
#include <functional>
#include <mutex>
#include <volk/volk_alloc.hh>

namespace satdump
{
    namespace ndsp
    {
        class FFTGenBlock : public Block
        {
        private:
            std::mutex fft_gen_mutex;
            volk::vector<float> fft_taps;
            int d_fft_size;
            int d_n;
            int d_fft_taps_size;
            uint32_t d_block_size;
            int lines = 0;
            float d_scale_min, d_scale_max;

            complex_t *copy_buffer;
            uint32_t copy_buffer_counter;

            bool work();

            void destroy_fft();

            complex_t *input_buffer;
            float *output_buffer = nullptr;

            int in_reshape_buffer = 0;
            complex_t *fft_reshape_buffer;

            int rbuffer_rate = 0;
            int rbuffer_size = 0;
            int rbuffer_skip = 0;

            int reshape_buffer_size = 0;

            fftwf_complex *fftw_in;
            fftwf_complex *fftw_out;
            fftwf_plan fftw_plan;

        public:
            FFTGenBlock();
            ~FFTGenBlock();
            void set_fft_gen_settings(int fft_size, int fft_overlap, uint32_t block_size);

            std::function<void(float *)> on_fft_gen_floats = [](float *) {};
            float *output_buff;

            bool d_fft_gen_end;

            nlohmann::json get_cfg(std::string key)
            {
                //
                throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump
