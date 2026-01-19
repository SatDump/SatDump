#pragma once

#include "dsp/flowgraph/flowgraph.h"
#include "handlers/handler.h"

#include "dsp/device/dev.h"
#include "utils/task_queue.h"

// TODOREWORK, move into plugin? Or Core?
namespace satdump
{
    namespace handlers
    {
        class DSPFlowGraphHandler : public Handler
        {
        public:
            DSPFlowGraphHandler();
            ~DSPFlowGraphHandler();

            ndsp::Flowgraph flowgraph;

            std::thread flow_thread;

            std::string to_add_var_name;

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);
            void drawMenuBar();

            TaskQueue tq;

            std::string getName() { return "Flowgraph"; }

            std::string getID() { return "dsp_flowgraph_handler"; }
        };
    } // namespace handlers
} // namespace satdump