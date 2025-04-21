#pragma once

#include "common/dsp/filter/firdes.h"
#include "dsp/block.h"
#include "dsp/filter/fir.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class RRC_FIRBlock : public FIRBlock<T>
        {
        private:
            double gain = 1;
            double samplerate = 6e6;
            double symbolrate = 2e6;
            double alpha = 0.35;
            int ntaps = 31;

        public:
            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "gain", "float", "Gain");
                add_param_simple(p, "samplerate", "float", "Samplerate");
                add_param_simple(p, "symbolrate", "float", "Symbolrate");
                add_param_simple(p, "alpha", "float", "Alpha");
                add_param_simple(p, "ntaps", "int", "NTaps");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "gain")
                    return gain;
                else if (key == "samplerate")
                    return samplerate;
                else if (key == "symbolrate")
                    return symbolrate;
                else if (key == "alpha")
                    return alpha;
                else if (key == "ntaps")
                    return ntaps;
                else
                    return FIRBlock<T>::get_cfg(key);
            }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "gain")
                    gain = v;
                else if (key == "samplerate")
                    samplerate = v;
                else if (key == "symbolrate")
                    symbolrate = v;
                else if (key == "alpha")
                    alpha = v;
                else if (key == "ntaps")
                    ntaps = v;

                if (key == "buffer_size")
                    return FIRBlock<T>::set_cfg(key, v);
                else
                {
                    auto taps = dsp::firdes::root_raised_cosine(gain, samplerate, symbolrate, alpha, ntaps);
                    return FIRBlock<T>::set_cfg("taps", taps);
                }
            }
        };
    } // namespace ndsp
} // namespace satdump