#include "flowgraph_processor.h"
#include "logger.h"

// TODOREWORK
#include "flowgraph/datasetproc_node.h"
#include "flowgraph/image_nodes.h"
#include "flowgraph/imageproduct_node.h"
#include "imgui/imnodes/imnodes.h"

namespace satdump
{
    namespace handlers
    {
        Flowgraph_DatasetProductProcessor::Flowgraph_DatasetProductProcessor(DatasetHandler *dh, Handler *dp, nlohmann::json p) : DatasetProductProcessor(dh, dp, p)
        {
            // TODOREWORK
            if (ImNodes::GetCurrentContext() == nullptr)
                ImNodes::CreateContext();

            flowgraph.node_internal_registry.emplace("image_product_source", []() { return std::make_shared<ImageProductSource_Node>(); });
            flowgraph.node_internal_registry.emplace("image_product_equation", []() { return std::make_shared<ImageProductExpression_Node>(); });
            flowgraph.node_internal_registry.emplace("image_sink", []() { return std::make_shared<ImageSink_Node>(); });
            flowgraph.node_internal_registry.emplace("image_get_proj", []() { return std::make_shared<ImageGetProj_Node>(); });
            flowgraph.node_internal_registry.emplace("image_reproj", []() { return std::make_shared<ImageReproj_Node>(); });
            flowgraph.node_internal_registry.emplace("image_equation", []() { return std::make_shared<ImageExpression_Node>(); });
            flowgraph.node_internal_registry.emplace("image_handler_sink", [dp]() { return std::make_shared<ImageHandlerSink_Node>(dp); });
            flowgraph.node_internal_registry.emplace("image_equalize", []() { return std::make_shared<ImageEqualize_Node>(); });
            flowgraph.node_internal_registry.emplace("image_source", []() { return std::make_shared<ImageSource_Node>(); });

            flowgraph.node_internal_registry.emplace("dataset_product_source", [this]() { return std::make_shared<DatasetProductSource_Node>(this); });

            if (p.contains("flowgraph"))
                flowgraph.setJSON(p["flowgraph"]);
        }

        nlohmann::json Flowgraph_DatasetProductProcessor::getCfg()
        {
            nlohmann::json cfg = DatasetProductProcessor::getCfg();
            cfg["flowgraph"] = flowgraph.getJSON();
            return cfg;
        }

        bool Flowgraph_DatasetProductProcessor::can_process()
        {
            return true; // TODOREWORK
        }

        void Flowgraph_DatasetProductProcessor::process(float *progress) { flowgraph.run(); }

        void Flowgraph_DatasetProductProcessor::renderUI() { flowgraph.render(); }
    } // namespace handlers
} // namespace satdump