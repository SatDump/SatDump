#include "image_handler.h"

#include "imgui/imgui_stdlib.h"
#include "products2/image/product_equation.h"
#include "common/image/processing.h" // TODOREWORK

namespace satdump
{
    void ImageViewerHandler2::init()
    {
        product = (products::ImageProduct *)ViewerHandler2::product;
    }

    ImageViewerHandler2::~ImageViewerHandler2()
    {
    }

    void ImageViewerHandler2::drawMenu()
    {
        if (ImGui::CollapsingHeader("Image"))
        {
            ImGui::InputText("##equation", &equation);
            if (ImGui::Button("Apply"))
            {
                auto fun = [this]()
                {
                    logger->critical("PID %llu", getpid());
                    auto img = products::generate_equation_product_composite(product, equation, &progress);
                    image::equalize(img, true);
                    image_view.update(img);
                };
                if (wip_thread)
                {
                    if (wip_thread->joinable())
                        wip_thread->join();
                    wip_thread.reset();
                }
                wip_thread = std::make_shared<std::thread>(fun);
            }
            ImGui::ProgressBar(progress);

            for (auto &ch : product->images)
            {
                ImGui::Separator();
                ImGui::Text("Channel %s", ch.channel_name.c_str());
                ch.ch_transform.render();
            }
        }
    }

    void ImageViewerHandler2::drawContents(ImVec2 win_size)
    {
        image_view.draw(win_size);
    }

    float ImageViewerHandler2::drawTreeMenu()
    {
        tree_local.start();

#if 0
        for (std::pair<std::string, ImageCompositeCfg> &compo : rgb_presets)
        {
            ImGui::TreeNodeEx(std::string("RGB " + compo.first).c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
            if (ImGui::IsItemClicked())
            {
                rgb_compo_cfg = compo.second;
                updateRGB();
            }
            tree_local.node();
        }
#endif

        return tree_local.end();
    }
}
