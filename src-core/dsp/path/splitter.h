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

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "noutputs")
                    return p_noutputs;
                throw satdump_exception(key);
            }

            void set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "noutputs")
                {
                    int no = p_noutputs;
                    p_noutputs = v;

                    if (no != p_noutputs)
                    {
                        Block::outputs.clear();
                        for (int i = 0; i < p_noutputs; i++)
                        {
                            Block::outputs.push_back({{"out", std::is_same_v<T, complex_t> ? DSP_SAMPLE_TYPE_CF32 : DSP_SAMPLE_TYPE_F32}});
                            outputs[i].fifo = std::make_shared<DspBufferFifo>(16); // TODOREWORK
                        }
                    }
                }
                else
                    throw satdump_exception(key);
            }
        };
    }
}