#pragma once

#include "common/dsp/complex.h"
#include "common/dsp/utils/random.h"
#include "dsp/block.h"
#include "dsp/block_simple.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        class ChannelModelSimpleBlock : public BlockSimple<complex_t, complex_t>
        {
        private:
            bool needs_reinit = true;

            double samplerate = 1e6;
            double noise_level = -150;
            double signal_level = -120;
            double freq_shift = 0;

            float curr_phase = 0, curr_freq = 0; // Current phase & freq
            float d_alpha = 0.1;                 // Rate at which we catch up with target_freq

            dsp::Random d_rng;

        public:
            ChannelModelSimpleBlock();
            ~ChannelModelSimpleBlock();

            uint32_t process(complex_t *input, uint32_t nsamples, complex_t *output);

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "samplerate", "float", "Samplerate");
                add_param_simple(p, "noise_level", "float", "Noise Level (dBm)");
                add_param_simple(p, "signal_level", "float", "Signal Level (dBm)");
                add_param_simple(p, "freq_shift", "float", "Frequency Shift");
                add_param_simple(p, "d_alpha", "float", "Alpha");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "samplerate")
                    return samplerate;
                else if (key == "noise_level")
                    return noise_level;
                else if (key == "signal_level")
                    return signal_level;
                else if (key == "freq_shift")
                    return freq_shift;
                else if (key == "alpha")
                    return d_alpha;
                else
                    throw satdump_exception(key);
            }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "samplerate")
                {
                    samplerate = v;
                    needs_reinit = true;
                }
                else if (key == "noise_level")
                {
                    noise_level = v;
                }
                else if (key == "signal_level")
                {
                    signal_level = v;
                }
                else if (key == "freq_shift")
                {
                    freq_shift = v;
                }
                else if (key == "alpha")
                {
                    d_alpha = v;
                }
                else
                    throw satdump_exception(key);
                return Block::RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump
