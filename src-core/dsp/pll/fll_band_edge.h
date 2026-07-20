#pragma once

#include "common/dsp/complex.h"
#include "dsp/block_simple.h"
#include <volk/volk.h>
#include <volk/volk_complex.h>
#include <volk/volk_malloc.h>

#define M_TWOPI (2 * M_PI)

namespace satdump
{
    namespace ndsp
    {
        class FIRBlockComplex
        {
        private:
            complex_t *buffer;
            complex_t *taps;
            int ntaps;
            void work();

        public:
            FIRBlockComplex(int maxbuf, std::vector<complex_t> taps)
            {
                // Set taps
                ntaps = taps.size();

                // Init taps
                this->taps = (complex_t *)volk_malloc((ntaps) * sizeof(complex_t), volk_get_alignment());
                for (int y = 0; y < ntaps; y++)
                    this->taps[y] = 0;
                for (int j = 0; j < ntaps; j++)
                    this->taps[j] = taps[(ntaps - 1) - j]; // Reverse taps

                // Init buffer
                buffer = (complex_t *)volk_malloc(2 * maxbuf * sizeof(complex_t), volk_get_alignment()); // TODO
            }

            ~FIRBlockComplex()
            {
                volk_free(taps);
                volk_free(buffer);
            }

            complex_t process(complex_t v)
            {
                memcpy(&buffer[ntaps], &v, 1 * sizeof(complex_t));

                complex_t vv;
                volk_32fc_x2_dot_prod_32fc((lv_32fc_t *)&vv, (lv_32fc_t *)&buffer[0 + 1], (lv_32fc_t *)taps, ntaps);

                memmove(&buffer[0], &buffer[1], ntaps * sizeof(complex_t));
                return vv;
            }
        };

        inline float sinc(float x)
        {
            // TODO: this seems to be numerically problematic for extremely long filters
            // TODO: investigate (-epsilon < x < epsilon)
            const float arg = static_cast<float>(M_PI) * x;
            return x == 0.0f ? 1.0f : std::sin(arg) / arg;
        }

        inline void sincosf(float x, float *sinx, float *cosx)
        {
            *sinx = ::sin(x);
            *cosx = ::cos(x);
        }

        inline complex_t gr_expj(float phase)
        {
            float t_imag, t_real;
            sincosf(phase, &t_imag, &t_real);
            return complex_t(t_real, t_imag);
        }

        class FLLBandEdgeBlock : public BlockSimple<complex_t, complex_t>
        {
        private:
            double max_freq;
            double min_freq;

            float d_phase, d_freq;

            float d_alpha, d_beta;

            std::unique_ptr<FIRBlockComplex> filter_up, filter_low;

            bool need_reinit = false;

        private:
            double samps_per_sym = 2;
            float rolloff = 0.5;
            int filter_size = 31;
            float loop_bw = 0.005;

        public:
            uint32_t process(complex_t *input, uint32_t nsamples, complex_t *output);

        public:
            FLLBandEdgeBlock();
            ~FLLBandEdgeBlock();

            void init()
            {
                max_freq = M_TWOPI * (2.0 / samps_per_sym);
                min_freq = -M_TWOPI * (2.0 / samps_per_sym);

                const int M = rintf(static_cast<float>(filter_size) / samps_per_sym);
                float power = 0.0f;

                // Create the baseband filter by adding two sincs together
                std::vector<float> bb_taps;
                bb_taps.reserve(filter_size);
                const float half_sps_inv = 2.0f / samps_per_sym;
                for (size_t i = 0; i < filter_size; i++)
                {
                    const float k = -M + i * half_sps_inv;
                    const float position = rolloff * k;
                    const float tap = sinc(position - 0.5f) + sinc(position + 0.5f);
                    power += tap * tap;

                    bb_taps.push_back(tap);
                }

                std::vector<complex_t> d_taps_lower;
                std::vector<complex_t> d_taps_upper;

                d_taps_lower.resize(filter_size);
                d_taps_upper.resize(filter_size);

                // Create the band edge filters by spinning the baseband
                // filter up and down to the right places in frequency.
                // Also, normalize the power in the filters
                using signed_type = std::make_signed<decltype(filter_size)>::type;
                const signed_type N = (bb_taps.size() - 1) / 2;
                const float invpower = 1.0f / power;
                const float inv_twice_sps = 0.5f / samps_per_sym;
                for (decltype(filter_size) i = 0; i < filter_size; i++)
                {
                    const float tap = bb_taps[i] * invpower;
                    const float k = (static_cast<signed_type>(i) - N) * inv_twice_sps;

                    const size_t index = filter_size - i - 1;
                    d_taps_lower[index] = gr_expj(-M_TWOPI * (1 + rolloff) * k) * tap;
                    d_taps_upper[index] = d_taps_lower[filter_size - i - 1].conj();
                }

                filter_low = std::make_unique<FIRBlockComplex>(1e6, d_taps_lower);
                filter_up = std::make_unique<FIRBlockComplex>(1e6, d_taps_upper);

                // Loop
                d_phase = 0, d_freq = 0;

                float damping = sqrtf(2.0f) / 2.0f;
                float denom = (1.0 + 2.0 * damping * loop_bw + loop_bw * loop_bw);
                d_alpha = (4 * damping * loop_bw) / denom;
                d_beta = (4 * loop_bw * loop_bw) / denom;
            }

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "samps_per_sym", "float");
                add_param_simple(p, "rolloff", "float");
                add_param_simple(p, "filter_size", "int");
                add_param_simple(p, "loop_bw", "float");
                add_param_simple(p, "freq", "stat");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "samps_per_sym")
                    return samps_per_sym;
                else if (key == "rolloff")
                    return rolloff;
                else if (key == "filter_size")
                    return filter_size;
                else if (key == "loop_bw")
                    return loop_bw;
                else if (key == "freq")
                    return d_freq;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "samps_per_sym")
                {
                    samps_per_sym = v;
                    need_reinit = true;
                }
                else if (key == "rolloff")
                {
                    rolloff = v;
                    need_reinit = true;
                }
                else if (key == "filter_size")
                {
                    filter_size = v;
                    need_reinit = true;
                }
                else if (key == "loop_bw")
                {
                    loop_bw = v;
                    need_reinit = true;
                }
                else if (key == "freq")
                    ;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump