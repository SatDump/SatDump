#pragma once

#include "dsp/block.h"
#include "dsp/block_simple.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class BlankerBlock : public BlockSimple<T, T>
        {
        private:
            bool blank = false;

        public:
            BlankerBlock();
            ~BlankerBlock();

            uint32_t process(T *input, uint32_t nsamples, T *output);

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "blank", "bool", "Blank");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "blank")
                    return blank;
                else
                    throw satdump_exception(key);
            }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "blank")
                    blank = v;
                else
                    throw satdump_exception(key);
                return Block::RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump
