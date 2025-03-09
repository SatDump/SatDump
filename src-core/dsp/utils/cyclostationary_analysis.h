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

            nlohmann::json getParams()
            {
                nlohmann::json v;

                return v;
            }

            void setParams(nlohmann::json v)
            {
                //  setValFromJSONIfExists(p_loop_bw, v["loop_bw"]);
            }
        };
    }
}