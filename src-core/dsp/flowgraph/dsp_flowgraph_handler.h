#pragma once

#include "dsp/flowgraph/context_wrapper.h"
#include "dsp/flowgraph/flowgraph.h"
#include "handlers/handler.h"

#include "dsp/device/dev.h"
#include "imgui/imnodes/imnodes_internal.h"
#include "utils/task_queue.h"

// TODOREWORK, move into plugin? Or Core?
namespace satdump
{
    namespace handlers
    {
        class DSPFlowGraphHandler : public Handler
        {
        public:
            DSPFlowGraphHandler(std::string file = "");
            ~DSPFlowGraphHandler();

            ImNodesContext *imnode_ctx = nullptr;
            ContainedContext ctx;

            std::string current_file = "";

            ndsp::Flowgraph flowgraph;

            std::thread flow_thread;

            std::string to_add_var_name;

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

            TaskQueue tq;

            std::string getName() { return current_file == "" ? "DSP Flowgraph" : std::filesystem::path(current_file).stem().string(); }

            std::string getID() { return "dsp_flowgraph_handler"; }
        };
    } // namespace handlers
} // namespace satdump