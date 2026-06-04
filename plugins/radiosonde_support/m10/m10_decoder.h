#pragma once

#include "dsp/block.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        class M10DecoderBlock : public Block
        {
        private:
            uint8_t shift_buf[1664];
            uint8_t obuf2[208];
            uint32_t frm_cnt = 0;

        private:
            bool work();

        public:
            M10DecoderBlock();
            ~M10DecoderBlock();

            void init();

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "frm_cnt", "stat");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "frm_cnt")
                    return frm_cnt;
                else
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