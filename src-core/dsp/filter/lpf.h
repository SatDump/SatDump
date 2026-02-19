#pragma once

#include "common/dsp/filter/firdes.h"
#include "dsp/block.h"
#include "dsp/filter/fir.h"

namespace satdump
{

    namespace ndsp
    {

        template <typename T>
        class LPF_FIRBlock : public FIRBlock<T>
        {
        private:
            double gain = 1;
            double samplerate = 6e6;
            double cutoff = 2e6;
            double transition = 0.35;

        public:
            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "gain", "float", "Gain");
                add_param_simple(p, "samplerate", "float", "Samplerate");
                add_param_simple(p, "cutoff", "float", "Cutoff");
                add_param_simple(p, "transition", "float", "Transition");
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
                else if (key == "transition")
                    return transition;
                else
                    return FIRBlock<T>::get_cfg(key);
            }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "gain")
                    gain = v;
                else if (key == "samplerate")
                    samplerate = v;
                else if (key == "cutoff")
                    cutoff = v;
                else if (key == "transition")
                    transition = v;

                if (key == "buffer_size")
                    return FIRBlock<T>::set_cfg(key, v);
                else
                {
                    auto taps = dsp::firdes::low_pass(gain, samplerate, cutoff, transition);
                    return FIRBlock<T>::set_cfg("taps", taps);
                }
            }
        };

    } // namespace ndsp
} // namespace satdump
