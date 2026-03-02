#pragma once

#include "common/dsp/buffer.h"
#include "dsp/block.h"
#include <memory>

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class Old2NewBlock : public Block
        {
        private:
            bool work();

        public:
            bool old_stream_exit = false;
            std::shared_ptr<dsp::stream<T>> old_stream;

        public:
            Old2NewBlock();
            ~Old2NewBlock();

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;

                return p;
            }

            nlohmann::json get_cfg(std::string key) { throw satdump_exception(key); }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump