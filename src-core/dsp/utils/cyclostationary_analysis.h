#pragma once

#include "dsp/block.h"
#include "common/dsp/complex.h"

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

            void init()
            {
            }

            nlohmann::json get_cfg()
            {
                nlohmann::json v;

                return v;
            }

            void set_cfg(nlohmann::json v)
            {
                //  setValFromJSONIfExists(p_loop_bw, v["loop_bw"]);
            }
        };
    }
}