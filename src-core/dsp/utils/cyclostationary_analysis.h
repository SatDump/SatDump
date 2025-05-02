#pragma once

#include "common/dsp/complex.h"
#include "dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        class CyclostationaryAnalysis : public Block
        {
        public:
            //

        private:
            bool work();

        public:
            CyclostationaryAnalysis();
            ~CyclostationaryAnalysis();

            void init() {}

            nlohmann::json get_cfg(std::string key)
            {
                throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                throw satdump_exception(key);
                return RES_OK;
                 // setValFromJSONIfExists(p_loop_bw, v["loop_bw"]);
            }
        };
    } // namespace ndsp
} // namespace satdump
