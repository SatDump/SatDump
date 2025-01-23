#pragma once

#include "flowgraph.h"
#include "../dataset_product_handler.h"
#include "../../image/image_handler.h"

namespace satdump
{
    class ImageHandlerSink_Node : public NodeInternal
    {
    private:
        viewer::DatasetProductHandler *ptr;

    public:
        ImageHandlerSink_Node(viewer::DatasetProductHandler *ptr)
            : NodeInternal("Image Handler Sink"), ptr(ptr)
        {
            inputs.push_back({"Image"});
        }

        void process()
        {
            std::shared_ptr<image::Image> img_pro = std::static_pointer_cast<image::Image>(inputs[0].ptr);

            auto handler = std::make_shared<viewer::ImageHandler>(*img_pro);
            ptr->addSubHandler(handler);

            has_run = true;
        }

        void render() {}
        nlohmann::json to_json() { return {}; }
        void from_json(nlohmann::json j) {}
    };
}