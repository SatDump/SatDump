#pragma once

#include "common/dsp/complex.h"
#include "common/dsp/resamp/polyphase_bank.h"
#include "dsp/block.h"
#include "dsp/block_simple.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class RationalResamplerBlock : public BlockSimple<T, T>
        {
        public:
            float p_interpolation = 1;
            float p_decimation = 2;
            std::vector<float> p_taps = {};

            bool needs_reinit = false;

        private:
            // Buffer
            T *buffer = nullptr;

            // Settings
            int d_interpolation;
            int d_decimation;
            int d_ctr;

            int inc = 0, outc = 0;

            // Taps
            dsp::PolyphaseBank pfb;

        public:
            uint32_t process(T *input, uint32_t nsamples, T *output);

        public:
            RationalResamplerBlock();
            ~RationalResamplerBlock();

            void init();

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "interpolation", "float");
                add_param_simple(p, "decimation", "float");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "interpolation")
                    return p_interpolation;
                else if (key == "decimation")
                    return p_decimation;
                else
                    throw satdump_exception(key);
            }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "interpolation")
                {
                    p_interpolation = v;
                    needs_reinit = true;
                }
                else if (key == "decimation")
                {
                    p_decimation = v;
                    needs_reinit = true;
                }
                else
                    throw satdump_exception(key);
                return Block::RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump