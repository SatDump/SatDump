#pragma once

#include "common/dsp/filter/firdes.h"
#include "dsp/block.h"
#include "dsp/filter/fft.h"
#include "dsp/filter/fir.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class LPF_Block : public T
        {
        private:
            double gain = 1;
            double samplerate = 6e6;
            double cutoff = 2e6;
            double transition_width = 100000;

        public:
            LPF_Block() : T("lpf") {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "gain", "float", "Gain");
                add_param_simple(p, "samplerate", "float", "Samplerate");
                add_param_simple(p, "cutoff", "float", "Cutoff");
                add_param_simple(p, "transition_width", "float", "Transition Width");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "gain")
                    return gain;
                else if (key == "samplerate")
                    return samplerate;
                else if (key == "cutoff")
                    return cutoff;
                else if (key == "transition_width")
                    return transition_width;
                else
                    return T::get_cfg(key);
            }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "gain")
                    gain = v;
                else if (key == "samplerate")
                    samplerate = v;
                else if (key == "cutoff")
                    cutoff = v;
                else if (key == "transition_width")
                    transition_width = v;

                if (key == "buffer_size")
                    return T::set_cfg(key, v);
                else
                {
                    auto taps = dsp::firdes::low_pass(gain, samplerate, cutoff, transition_width);
                    return T::set_cfg("taps", taps);
                }
            }
        };
    } // namespace ndsp
} // namespace satdump