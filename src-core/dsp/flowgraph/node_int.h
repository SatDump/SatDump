#pragma once

/**
 * @file node_int.h
 * @brief DSP Flowgraph NodeInternal implementation (the "guts" of nodes)
 */

#include "dsp/block.h"
#include "dsp/device/options_displayer_warper.h"
#include "nlohmann/json.hpp"

namespace satdump
{
    namespace ndsp
    {
        namespace flowgraph
        {
            // Must be declared already...
            class Flowgraph;

            /**
             * @brief Class holding the actual guts
             * of a node. This was made to avoid
             * having the entire node class be overloaded
             * as DSP Nodes should not have access to
             * internal flowgraph logic.
             *
             * This therefore by default only handles a simple
             * block, showing basic options/stats and IO. More
             * advanced displays require overloading this class
             * and adding custom rendering (or more?) logic.
             */
            class NodeInternal
            {
            protected:
                //! @brief the flowgraph this node belongs to
                Flowgraph *f;

                //! @brief OptDisplayer to display block configurations
                std::shared_ptr<ndsp::OptDisplayerWarper> optdisp;

                //! @brief if true, consider this node as having some kind of error to show
                bool is_error = false;

            public:
                //! @brief the actual dsp Block
                std::shared_ptr<ndsp::Block> blk;

            public:
                /**
                 * @brief Constructor of a NodeInternal
                 * @param f flowgrapgh the node will belong to
                 * @param b actual DSP block
                 */
                NodeInternal(const Flowgraph *f, std::shared_ptr<ndsp::Block> b);

                /**
                 * @brief Render the node's GUI
                 */
                virtual bool render();

                /**
                 * @brief Update optional displayer state, to
                 * update the display accordingly after starting/stopping
                 * the node/block.
                 */
                virtual void upd_state() { optdisp->update(); }

                /**
                 * @brief get block parameter(s)
                 * @return the parameter(s)
                 */
                virtual nlohmann::json getP();

                /**
                 * @brief set block parameter(s) (and update the GUI)
                 * @param p the parameter(s)
                 */
                virtual void setP(nlohmann::json p);

                /**
                 * @brief get if a node has an error
                 * @return true on error
                 */
                virtual bool is_errored() { return is_error; }
            };
        } // namespace flowgraph
    } // namespace ndsp
} // namespace satdump