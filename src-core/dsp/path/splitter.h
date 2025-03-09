#pragma once

#include "dsp/block.h"
#include "common/dsp/complex.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        class SplitterBlock : public Block
        {
        public:
            int p_noutputs = 1;

        private:
            bool work();

        public:
            SplitterBlock();
            ~SplitterBlock();

            void init()
            {
            }

            nlohmann::json getParams()
            {
                nlohmann::json v;
                v["noutputs"] = p_noutputs;
                return v;
            }

            void setParams(nlohmann::json v)
            {
                int no = p_noutputs;
                setValFromJSONIfExists(p_noutputs, v["noutputs"]);
                if (no != p_noutputs)
                {
                    Block::outputs.clear();
                    for (int i = 0; i < p_noutputs; i++)
                    {
                        Block::outputs.push_back({{"out", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_COMPLEX : DSP_SAMPLE_TYPE_FLOAT}});
                        outputs[i].fifo = std::make_shared<DspBufferFifo>(16); // TODOREWORK
                    }
                }
            }
        };
    }
}