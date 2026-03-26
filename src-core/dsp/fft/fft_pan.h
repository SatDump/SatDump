#pragma once

#include "common/dsp/complex.h"
#include "dsp/block.h"
#include <fftw3.h>
#include <functional>
#include <mutex>
#include <volk/volk_alloc.hh>

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
            int p_size = 65536;
            uint64_t p_samplerate = 10e6;
            int p_rate = 20;

        public:
            FFTPanBlock();
            ~FFTPanBlock();
            void set_fft_settings(int size, uint64_t samplerate, int rate = 60);

            std::function<void(float *, size_t)> on_fft = [](float *, size_t size) {};

            float *output_fft_buff;

            float avg_num = 10;

            void init() { set_fft_settings(p_size, p_samplerate, p_rate); }

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_list(p, "size", "int", {8192, 16384, 32768, 65536, 131072}, "FFT Size");
                add_param_simple(p, "samplerate", "float");
                add_param_simple(p, "rate", "int", "FFT Rate");
                add_param_simple(p, "avg_num", "float", "Avg Num");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "size")
                    return fft_size;
                else if (key == "samplerate")
                    return p_samplerate;
                else if (key == "rate")
                    return p_rate;
                else if (key == "avg_num")
                    return avg_num;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "size" || key == "samplerate" || key == "rate")
                {
                    if (key == "size")
                        p_size = v;
                    else if (key == "samplerate")
                        p_samplerate = v;
                    else if (key == "rate")
                        p_rate = v;

                    set_fft_settings(p_size, p_samplerate, p_rate);
                }
                else if (key == "avg_num")
                    avg_num = v;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump