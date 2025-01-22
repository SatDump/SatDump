#pragma once

#include "flowgraph.h"
#include "common/image/io.h"
#include "common/image/meta.h"
#include "projection/projection.h"
#include "projection/reprojector.h"

namespace satdump
{
    class ImageSink_Node : public NodeInternal
    {
    private:
        std::string path;

    public:
        ImageSink_Node()
            : NodeInternal("Image Sink")
        {
            inputs.push_back({"Image"});
        }

        void process()
        {
            std::shared_ptr<image::Image> img = std::static_pointer_cast<image::Image>(inputs[0].ptr);

            image::save_img(*img, path);

            has_run = true;
        }

        void render()
        {
            ImGui::SetNextItemWidth(200 * ui_scale);
            ImGui::InputText("Path", &path);
        }

        nlohmann::json to_json()
        {
            nlohmann::json j;
            j["path"] = path;
            return j;
        }

        void from_json(nlohmann::json j)
        {
            path = j["path"];
        }
    };

    class ImageGetProj_Node : public NodeInternal
    {
    private:
        std::string path;

    public:
        ImageGetProj_Node()
            : NodeInternal("Image Get Projection")
        {
            inputs.push_back({"Image"});
            outputs.push_back({"Projection"});
        }

        void process()
        {
            std::shared_ptr<image::Image> img = std::static_pointer_cast<image::Image>(inputs[0].ptr);

            std::shared_ptr<proj::Projection> ptr = std::make_shared<proj::Projection>(image::get_metadata_proj_cfg(*img));
            ptr->height = img->height();
            ptr->width = img->width();
            outputs[0].ptr = ptr;

            has_run = true;
        }

        void render() {}
        nlohmann::json to_json() { return {}; }
        void from_json(nlohmann::json j) {}
    };

    class ImageReproj_Node : public NodeInternal
    {
    private:
        std::string path;

    public:
        ImageReproj_Node()
            : NodeInternal("Reproj Image")
        {
            inputs.push_back({"Image"});
            inputs.push_back({"Projection"});
            outputs.push_back({"Image"});
        }

        void process()
        {
            std::shared_ptr<image::Image> img = std::static_pointer_cast<image::Image>(inputs[0].ptr);
            std::shared_ptr<proj::Projection> proj = std::static_pointer_cast<proj::Projection>(inputs[1].ptr);

            std::shared_ptr<image::Image> img_out = std::make_shared<image::Image>();

            // TODOREWORK!!!!
            proj::ReprojectionOperation op;
            op.img = img.get();
            op.output_width = proj->width;
            op.output_height = proj->height;
            op.target_prj_info = *proj;
            //            op.target_prj_info["width"] = proj->width;
            //            op.target_prj_info["height"] = proj->height;
            auto cfg = image::get_metadata_proj_cfg(*op.img);
            cfg["width"] = img->width();
            cfg["height"] = img->height();
            image::set_metadata_proj_cfg(*op.img, cfg);
            *img_out = proj::reproject(op);

            outputs[0].ptr = img_out;

            has_run = true;
        }

        void render() {}
        nlohmann::json to_json() { return {}; }
        void from_json(nlohmann::json j) {}
    };
}