#pragma once

#include "common/dsp/complex.h"
#include "dsp/block_simple.h"

namespace satdump
{
    namespace ndsp
    {
        class ShortToFloatBlock : public BlockSimple<int16_t, float>
        {
        private:
            float scale = 1.0;

        public:
            uint32_t process(int16_t *input, uint32_t nsamples, float *output);

        public:
            ShortToFloatBlock();
            ~ShortToFloatBlock();

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "scale", "float");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "scale")
                    return scale;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "scale")
                    scale = v;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump