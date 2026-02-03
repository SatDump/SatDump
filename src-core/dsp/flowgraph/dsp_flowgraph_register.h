#pragma once

#include "dsp/flowgraph/flowgraph.h"

namespace satdump
{
    namespace ndsp
    {
        template <typename T>
        void registerNodeSimple(ndsp::Flowgraph &flowgraph, std::string menuname)
        {
            std::string id = T().d_id;
            flowgraph.node_internal_registry.insert({id, {menuname, [](const ndsp::Flowgraph *f) { return std::make_shared<ndsp::NodeInternal>(f, std::make_shared<T>()); }}});
        }

        template <typename T>
        void registerNode(ndsp::Flowgraph &flowgraph, std::string menuname)
        {
            std::string id = T(&flowgraph).blk->d_id;
            flowgraph.node_internal_registry.insert({id, {menuname, [](const ndsp::Flowgraph *f) { return std::make_shared<T>(f); }}});
        }

        struct RegisterNodesEvent
        {
            std::map<std::string, ndsp::Flowgraph::NodeInternalReg> &r;
        };

        void registerNodesInFlowgraph(ndsp::Flowgraph &flowgraph);
    } // namespace ndsp
} // namespace satdump