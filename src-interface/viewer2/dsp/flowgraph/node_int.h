#pragma once

#include "nlohmann/json.hpp"
#include "dsp/block.h"

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
            void getFinalJSON();
            void setupJSONParams()
            {
                v.clear();
                for (auto &ji : j.items())
                    v.emplace(ji.key(), ji.value().dump());
            }

        public:
            std::shared_ptr<ndsp::Block> blk;
            // nlohmann::json v;
            nlohmann::json j;
            std::map<std::string, std::string> v;

        public:
            NodeInternal(const Flowgraph *f, std::shared_ptr<ndsp::Block> b);

            virtual void render();

            virtual nlohmann::json getP();
            virtual void setP(nlohmann::json p);
            virtual void applyP();
        };
    }
}