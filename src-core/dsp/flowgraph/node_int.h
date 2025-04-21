#pragma once

#include "dsp/block.h"
#include "dsp/device/options_displayer_warper.h"
#include "nlohmann/json.hpp"

namespace satdump
{
    namespace ndsp
    {
        // TODOREWORK proper sub-namespace?
        class Flowgraph;

        class NodeInternal
        {
        protected:
            Flowgraph *f;

            std::shared_ptr<ndsp::OptDisplayerWarper> optdisp;

        public:
            std::shared_ptr<ndsp::Block> blk;

        public:
            NodeInternal(const Flowgraph *f, std::shared_ptr<ndsp::Block> b);

            virtual bool render();

            virtual void up_state() { optdisp->update(); }

            virtual nlohmann::json getP();
            virtual void setP(nlohmann::json p);
        };
    } // namespace ndsp
} // namespace satdump
