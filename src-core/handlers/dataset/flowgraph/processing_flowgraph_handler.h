#pragma once

/**
 * @file processing_flowgraph_handler.h
 * @brief Processing Flowgraph Handler for use in the GUI
 */

#include "handlers/dataset/flowgraph/flowgraph.h"
#include "handlers/handler.h"
#include "imgui/imnodes/imnodes_internal.h"
#include "utils/imgui_context_wrapper.h"
#include "utils/task_queue.h"

namespace satdump
{
    namespace handlers
    {
        /**
         * @brief Processing Flowgraph handler
         * Holds a flowgraph and handles all rendering
         * and controls.
         *
         * Also handles saving/loading graphs in CBOR format.
         */
        class ProcessingFlowGraphHandler : public Handler
        {
        public:
            /**
             * @brief Constructor, optionally loading an existing flowgraph.
             * @param file optional file to load
             */
            ProcessingFlowGraphHandler(std::string file = "");

            /**
             * @brief Destructor
             */
            ~ProcessingFlowGraphHandler();

        private:
            //! @brief Context for zoom
            ImNodesContext *imnode_ctx = nullptr;
            ContainedContext ctx;

            //! @brief Currently open file
            std::string current_file = "";

            //! @brief Actual flowgraph
            Flowgraph flowgraph;

            //! @brief Thread the flowgraph runs in
            std::thread flow_thread;

            //! @brief Task queue for async save/load etc
            TaskQueue tq;

            //! @brief Node search string
            std::string node_search = "";

        private:
            double start_time = 0;

        private:
            //! @brief Name of variable to add
            std::string to_add_var_name;

        public:
            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

            std::string getName() { return current_file == "" ? "Processing Flowgraph" : std::filesystem::path(current_file).stem().string(); }

            std::string getID() { return "processing_flowgraph_handler"; }
        };
    } // namespace handlers
} // namespace satdump