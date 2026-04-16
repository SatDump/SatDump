#pragma once

#include "common/dsp/block.h"
#include "common/dsp/complex.h"
#include "dsp/block.h"
#include "dsp/block_simple.h"
#include "dsp/complex_json.h"
#include <fftw3.h>
#include <volk/volk_alloc.hh>

namespace satdump
{
    namespace ndsp
    {
        template <typename T, typename TT = float>
        class FFTFilterBlock : public Block
        {
        public:
            bool needs_reinit = false;
            std::vector<TT> p_taps;

        private:
            int buffer_size = 0;
            volk::vector<T> buffer;

            T *tail = nullptr;
            T *ffttaps_buffer = nullptr;

            size_t in_buffer;

            int d_ntaps = 0;
            int d_fftsize = 0;
            int d_nsamples = 0;

            fftwf_complex *fft_fwd_in = nullptr, *fft_fwd_out = nullptr;
            fftwf_plan fft_fwd;
            fftwf_complex *fft_inv_in = nullptr, *fft_inv_out = nullptr;
            fftwf_plan fft_inv;

        private:
            bool work();

        public:
            FFTFilterBlock(std::string id = "");
            ~FFTFilterBlock();

            void init();

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "taps")
                    return p_taps;
                else
                    throw satdump_exception(key);
            }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "taps")
                {
                    p_taps = v.get<std::vector<TT>>();
                    needs_reinit = true;
                }
                else
                    throw satdump_exception(key);
                return Block::RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump