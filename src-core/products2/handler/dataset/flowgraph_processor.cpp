#include "flowgraph_processor.h"
#include "libs/sol2/sol.hpp"
#include "logger.h"

#include "lua/lua_bind.h"

// TODOREWORK
#include "imgui/imnodes/imnodes.h"
#include "flowgraph/datasetproc_node.h"
#include "flowgraph/image_nodes.h"
#include "flowgraph/imageproduct_node.h"

namespace satdump
{
    namespace viewer
    {
        Flowgraph_DatasetProductProcessor::Flowgraph_DatasetProductProcessor(DatasetHandler *dh, Handler *dp, nlohmann::json p)
            : DatasetProductProcessor(dh, dp, p)
        {
            // TODOREWORK
            if (ImNodes::GetCurrentContext() == nullptr)
                ImNodes::CreateContext();

            flowgraph.node_internal_registry.emplace("image_product_source", []()
                                                     { return std::make_shared<ImageProductSource_Node>(); });
            flowgraph.node_internal_registry.emplace("image_product_equation", []()
                                                     { return std::make_shared<ImageProductEquation_Node>(); });
            flowgraph.node_internal_registry.emplace("image_sink", []()
                                                     { return std::make_shared<ImageSink_Node>(); });
            flowgraph.node_internal_registry.emplace("image_get_proj", []()
                                                     { return std::make_shared<ImageGetProj_Node>(); });
            flowgraph.node_internal_registry.emplace("image_reproj", []()
                                                     { return std::make_shared<ImageReproj_Node>(); });
            flowgraph.node_internal_registry.emplace("image_equation", []()
                                                     { return std::make_shared<ImageEquation_Node>(); });
            flowgraph.node_internal_registry.emplace("image_handler_sink", [dp]()
                                                     { return std::make_shared<ImageHandlerSink_Node>(dp); });
            flowgraph.node_internal_registry.emplace("image_equalize", []()
                                                     { return std::make_shared<ImageEqualize_Node>(); });
            flowgraph.node_internal_registry.emplace("image_source", []()
                                                     { return std::make_shared<ImageSource_Node>(); });

            if (p.contains("flowgraph"))
                flowgraph.setJSON(p["flowgraph"]);
        }

        nlohmann::json Flowgraph_DatasetProductProcessor::getCfg()
        {
            nlohmann::json cfg;
            cfg["flowgraph"] = flowgraph.getJSON();
            cfg["cfg"] = params;
            return cfg;
        }

        bool Flowgraph_DatasetProductProcessor::can_process()
        {
            return true; // TODOREWORK
        }

        void Flowgraph_DatasetProductProcessor::process(float *progress)
        {
            flowgraph.run();
        }

        void Flowgraph_DatasetProductProcessor::renderUI()
        {
            flowgraph.render();
        }
    }
}