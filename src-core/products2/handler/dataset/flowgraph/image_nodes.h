#pragma once

#include "flowgraph.h"
#include "common/image/io.h"
#include "common/image/meta.h"
#include "projection/projection.h"
#include "projection/reprojector.h"
#include "common/image/equation.h"
#include "common/image/processing.h"

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

    class ImageSource_Node : public NodeInternal
    {
    private:
        std::string product_path;

    public:
        ImageSource_Node()
            : NodeInternal("Image Source")
        {
            outputs.push_back({"Image"});
        }

        void process()
        {
            std::shared_ptr<image::Image> img_out = std::make_shared<image::Image>();
            image::load_img(*img_out, product_path);
            outputs[0].ptr = img_out;

            has_run = true;
        }

        void render()
        {
            ImGui::SetNextItemWidth(200 * ui_scale);
            ImGui::InputText("Path", &product_path);
        }

        nlohmann::json to_json()
        {
            nlohmann::json j;
            j["path"] = product_path;
            return j;
        }

        void from_json(nlohmann::json j)
        {
            product_path = j["path"];
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

    class ImageEquation_Node : public NodeInternal
    {
    private:
        struct ChConfig
        {
            std::string input_name;
            std::string tokens;

            NLOHMANN_DEFINE_TYPE_INTRUSIVE(ChConfig, input_name, tokens);
        };

        std::vector<ChConfig> channels;
        std::string equation;

    public:
        ImageEquation_Node()
            : NodeInternal("Image Equation")
        {
            outputs.push_back({"Image"});
        }

        void process()
        {
            std::vector<image::EquationChannel> ch;
            for (int i = 0; i < channels.size(); i++)
            {
                std::shared_ptr<image::Image> img = std::static_pointer_cast<image::Image>(inputs[i].ptr);
                ch.push_back({channels[i].tokens, img.get()});
                logger->warn(channels[i].tokens);
            }

            std::shared_ptr<image::Image> img_out = std::make_shared<image::Image>();
            *img_out = image::generate_image_equation(ch, equation);
            outputs[0].ptr = img_out;

            has_run = true;
        }

        void render()
        {
            ImGui::SetNextItemWidth(200 * ui_scale);
            ImGui::InputTextMultiline("Equation", &equation);

            for (auto &c : channels)
            {
                ImGui::Separator();
                ImGui::Text(c.input_name.c_str());
                ImGui::SameLine();
                ImGui::SetNextItemWidth(200 * ui_scale);
                ImGui::InputText(std::string("##input" + c.input_name).c_str(), &c.tokens);
            }
            ImGui::Separator();

            if (ImGui::Button("Add"))
            {
                std::string name = "Img" + std::to_string(channels.size() + 1);
                addInputDynamic({name});
                channels.push_back({name, "chimg" + std::to_string(channels.size() + 1)});
            }
        }

        nlohmann::json to_json()
        {
            nlohmann::json j;
            j["equation"] = equation;
            j["channels"] = channels;
            return j;
        }

        void from_json(nlohmann::json j)
        {
            equation = j["equation"];
            channels = j["channels"];
            inputs.clear();
            for (auto &c : channels)
                inputs.push_back({c.input_name});
        }
    };

    class ImageEqualize_Node : public NodeInternal
    {
    private:
        bool per_channel = false;

    public:
        ImageEqualize_Node()
            : NodeInternal("Equalize Image")
        {
            inputs.push_back({"Image"});
            outputs.push_back({"Image"});
        }

        void process()
        {
            std::shared_ptr<image::Image> img = std::static_pointer_cast<image::Image>(inputs[0].ptr);
            image::equalize(*img, per_channel);
            outputs[0].ptr = img;

            has_run = true;
        }

        void render()
        {
            ImGui::Checkbox("Per Channel", &per_channel);
        }

        nlohmann::json to_json()
        {
            nlohmann::json j;
            j["per_channel"] = per_channel;
            return j;
        }

        void from_json(nlohmann::json j)
        {
            per_channel = j["per_channel"];
        }
    };
}