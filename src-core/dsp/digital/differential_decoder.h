#pragma once

#include "common/codings/differential/nrzm.h"
#include "dsp/block_simple.h"

namespace satdump
{
    namespace ndsp
    {
        class DifferentialDecoderBlock : public BlockSimple<int8_t, int8_t>
        {
        private:
            diff::NRZMDiff nrzm;

        public:
            uint32_t process(int8_t *input, uint32_t nsamples, int8_t *output);

        public:
            DifferentialDecoderBlock();
            ~DifferentialDecoderBlock();

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                return p;
            }

            nlohmann::json get_cfg(std::string key) { throw satdump_exception(key); }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {

                throw satdump_exception(key);
                return Block::RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump