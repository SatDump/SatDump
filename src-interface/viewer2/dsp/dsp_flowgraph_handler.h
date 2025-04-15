#pragma once

#include "flowgraph/flowgraph.h"
#include "handler/handler.h"

// TODOREWORK, move into plugin? Or Core?
namespace satdump
{
    namespace viewer
    {
        class DSPFlowGraphHandler : public Handler
        {
        public:
            DSPFlowGraphHandler();
            ~DSPFlowGraphHandler();

            ndsp::Flowgraph flowgraph;

            std::thread flow_thread;

            // The Rest
            void drawMenu();
            void drawContents(ImVec2 win_size);

            std::string getName() { return "Flowgraph"; }

            std::string getID() { return "dsp_flowgraph_handler"; }
        };
    } // namespace viewer
} // namespace satdump
