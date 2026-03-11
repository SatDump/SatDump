#pragma once

/**
 * @file dsp_flowgraph_handler.h
 * @brief DSP Flowgraph Handler for use in the GUI
 */

#include "dsp/flowgraph/flowgraph.h"
#include "handlers/handler.h"
#include "imgui/imnodes/imnodes_internal.h"
#include "utils/imgui_context_wrapper.h"
#include "utils/task_queue.h"

namespace satdump
{
    namespace handlers
    {
        /**
         * @brief DSP Flowgraph handler
         * Holds a flowgraph and handles all rendering
         * and controls.
         *
         * Also handles saving/loading graphs in CBOR format.
         */
        class DSPFlowGraphHandler : public Handler
        {
        public:
            /**
             * @brief Constructor, optionally loading an existing flowgraph.
             * @param file optional file to load
             */
            DSPFlowGraphHandler(std::string file = "");

            /**
             * @brief Destructor
             */
            ~DSPFlowGraphHandler();

        private:
            //! @brief Context for zoom
            ImNodesContext *imnode_ctx = nullptr;
            ContainedContext ctx;

            //! @brief Currently open file
            std::string current_file = "";

            //! @brief Actual flowgraph
            ndsp::flowgraph::Flowgraph flowgraph;

            //! @brief Thread the flowgraph runs in
            std::thread flow_thread;

            //! @brief Task queue for async save/load etc
            TaskQueue tq;

            //! @brief Node search string
            std::string node_search = "";

        private:
            //! @brief Name of variable to add
            std::string to_add_var_name;

        public:
            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

            std::string getName() { return current_file == "" ? "DSP Flowgraph" : std::filesystem::path(current_file).stem().string(); }

            std::string getID() { return "dsp_flowgraph_handler"; }
        };
    } // namespace handlers
} // namespace satdump