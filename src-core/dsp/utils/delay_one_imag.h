#pragma once

#include "common/dsp/complex.h"
#include "dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        class DelayOneImagBlock : public Block
        {
        private:
        //

        private:
            float lastSamp;
            // complex_t tmp_val;

            bool work();

        public:
            DelayOneImagBlock();
            ~DelayOneImagBlock();

            void init() {}

            nlohmann::json get_cfg(std::string key)
            {
                throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                throw satdump_exception(key);
                return RES_OK;
            }
        };

    } // namespace ndsp
} // namespace satdump
