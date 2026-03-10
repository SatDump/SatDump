#pragma once

/**
 * @file dsp_flowgraph_register.h
 * @brief Registers nodes into a flowgraph
 */

#include "dsp/flowgraph/flowgraph.h"

namespace satdump
{
    namespace ndsp
    {
        namespace flowgraph
        {
            /**
             * @brief Register a basic node in flowgraph.
             * That is, register a simple Block without any further modifications
             * @param T the dsp Block to register
             * @param flowgraph flowgraph to register into
             * @param menuname name in the menu, with submenus
             */
            template <typename T>
            void registerNodeSimple(Flowgraph &flowgraph, std::string menuname)
            {
                std::string id = T().d_id;
                flowgraph.node_internal_registry.insert({id, {menuname, [](const Flowgraph *f) { return std::make_shared<NodeInternal>(f, std::make_shared<T>()); }}});
            }

            /**
             * @brief Register a complex node in flowgraph.
             * That is, a node that must have a non-stock
             * NodeInternal (eg, custom rendering)
             * @param T the NodeInternal to register
             * @param flowgraph flowgraph to register into
             * @param menuname name in the menu, with submenus
             */
            template <typename T>
            void registerNode(Flowgraph &flowgraph, std::string menuname)
            {
                std::string id = T(&flowgraph).blk->d_id;
                flowgraph.node_internal_registry.insert({id, {menuname, [](const Flowgraph *f) { return std::make_shared<T>(f); }}});
            }

            /**
             * @brief Event called to register additional nodes in flowgraph
             * @param r node registry
             */
            struct RegisterNodesEvent
            {
                std::map<std::string, Flowgraph::NodeInternalReg> &r;
            };

            /**
             * @brief Register all possible nodes into a flowgraph
             * @param flowgraph the flowgraph
             */
            void registerNodesInFlowgraph(Flowgraph &flowgraph);
        } // namespace flowgraph
    } // namespace ndsp
} // namespace satdump