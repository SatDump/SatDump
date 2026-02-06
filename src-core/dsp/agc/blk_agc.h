#pragma once

#include "dsp/block_simple.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class BlkAGCBlock : public BlockSimple<T, T>
        {
        public:
            float p_rate = 1e-4;
            float p_reference = 1.0;
            float p_gain = 1.0;
            float p_max_gain = 65536;

        private:
            float rate;      // adjustment rate
            float reference; // reference value
            float gain;      // current gain
            float max_gain;  // max allowable gain

        public:
            uint32_t process(T *input, uint32_t nsamples, T *output);

        public:
            BlkAGCBlock();
            ~BlkAGCBlock();

            void init()
            {
                rate = p_rate;
                reference = p_reference;
                gain = p_gain;
                max_gain = p_max_gain;
            }

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "rate", "float", "Rate");
                add_param_simple(p, "reference", "float", "Reference");
                add_param_simple(p, "gain", "float", "Gain");
                add_param_simple(p, "max_gain", "float", "Max Gain");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "rate")
                    return p_rate;
                else if (key == "reference")
                    return p_reference;
                else if (key == "gain")
                    return p_gain;
                else if (key == "max_gain")
                    return p_max_gain;
                else
                    throw satdump_exception(key);
            }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "rate")
                    p_rate = v;
                else if (key == "reference")
                    p_reference = v;
                else if (key == "gain")
                    p_gain = v;
                else if (key == "max_gain")
                    p_max_gain = v;
                else
                    throw satdump_exception(key);
                init();
                return Block::RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump