#pragma once

#include "core/exception.h"
#include "flowgraph.h"
#include "products/image/product_expression.h"
#include "products/image_product.h"
#include "products/product.h"

namespace satdump
{
    class ImageProductSource_Node : public NodeInternal
    {
    private:
        std::string product_path;

    public:
        ImageProductSource_Node() : NodeInternal("Image Product Source") { outputs.push_back({"Product", "product"}); }

        void process()
        {
            outputs[0].ptr = products::loadProduct(product_path); //"/home/alan/Downloads/SatDump_NEWPRODS/metop_test/AVHRR/product.cbor");

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

        void from_json(nlohmann::json j) { product_path = j["path"]; }
    };

    class ImageProductExpression_Node : public NodeInternal
    {
    private:
        std::string expression;

        bool processing = false;
        float progress = 0;

    public:
        ImageProductExpression_Node() : NodeInternal("Image Product Expression")
        {
            inputs.push_back({"Product", "product"});
            outputs.push_back({"Image", "img"});
        }

        void process()
        {
            processing = true;

            if (std::static_pointer_cast<satdump::products::Product>(inputs[0].ptr)->type == "image")
            {
                std::shared_ptr<satdump::products::ImageProduct> img_pro = std::static_pointer_cast<satdump::products::ImageProduct>(inputs[0].ptr);

                std::shared_ptr<image::Image> img_out = std::make_shared<image::Image>();
                *img_out = products::generate_expression_product_composite(img_pro.get(), expression, &progress);

                outputs[0].ptr = img_out;
            }
            else
            {
                throw satdump_exception("Must be an image product!");
            }

            has_run = true;
            processing = false;
        }

        void render()
        {
            ImGui::SetNextItemWidth(200 * ui_scale);
            ImGui::InputTextMultiline("Expression", &expression);

            if (processing)
                ImGui::ProgressBar(progress, {200 * ui_scale, 0});
        }

        nlohmann::json to_json()
        {
            nlohmann::json j;
            j["expression"] = expression;
            return j;
        }

        void from_json(nlohmann::json j) { expression = j["expression"]; }
    };
} // namespace satdump