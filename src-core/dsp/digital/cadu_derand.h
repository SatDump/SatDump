#pragma once

#include "dsp/block.h"
#include "dsp/block_simple.h"

namespace satdump
{
    namespace ndsp
    {
        class CADUDerandBlock : public BlockSimple<int8_t, int8_t>
        {
        private:
            int cadu_size = 1024;
            int offset = 4;

        public:
            uint32_t process(int8_t *input, uint32_t nsamples, int8_t *output);

        public:
            CADUDerandBlock();
            ~CADUDerandBlock();

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "cadu_size", "int");
                add_param_simple(p, "offset", "int");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "cadu_size")
                    return cadu_size;
                else if (key == "offset")
                    return offset;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "cadu_size")
                    cadu_size = v;
                else if (key == "offset")
                    offset = v;
                else
                    throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump