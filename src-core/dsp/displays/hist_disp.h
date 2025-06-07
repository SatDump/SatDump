#pragma once

#include "common/dsp/complex.h"
#include "common/widgets/histogram.h"
#include "dsp/block.h"

namespace satdump
{
    namespace ndsp
    {
        class HistogramDisplayBlock : public Block
        {
        public:
            widgets::HistoViewer histo;

        public:
            HistogramDisplayBlock();
            ~HistogramDisplayBlock();

            bool work();

            nlohmann::json get_cfg(std::string key)
            {
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
