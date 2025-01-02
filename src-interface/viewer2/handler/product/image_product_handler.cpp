#include "image_product_handler.h"

#include "imgui/imgui_stdlib.h"
#include "products2/image/product_equation.h"
#include "common/image/processing.h" // TODOREWORK

namespace satdump
{
    namespace viewer
    {
        ImageProductHandler::ImageProductHandler(std::shared_ptr<products::Product> p)
            : ProductHandler(p)
        {
            product = (products::ImageProduct *)ProductHandler::product.get();
            for (auto &img : product->images)
                channel_selection_box_str += "Channel " + img.channel_name + '\0';
        }

        ImageProductHandler::~ImageProductHandler()
        {
        }

        void ImageProductHandler::drawMenu()
        {
            bool needs_to_be_disabled = is_processing;

            if (ImGui::CollapsingHeader("Channels"))
            {
                if (needs_to_be_disabled)
                    style::beginDisabled();

                needs_to_update |= ImGui::Combo("##imageproductchannelcombo", &channel_selection_curr_id, channel_selection_box_str.c_str());

                if (channel_selection_curr_id != -1)
                {
                    auto &ch = product->images[channel_selection_curr_id];

                    if (ch.wavenumber != -1)
                    {
                        ImGui::Text("Wavenumber : %f", ch.wavenumber);
                    }
                }

                if (needs_to_be_disabled)
                    style::endDisabled();
            }

            needs_to_update |= renderPresetMenu(); // TODOREWORK move in top drawMenu?

            if (ImGui::CollapsingHeader("Equation"))
            {
                if (needs_to_be_disabled)
                    style::beginDisabled();

                ImGui::InputText("##equation", &equation);
                if (ImGui::Button("Apply"))
                {
                    channel_selection_curr_id = -1;
                    needs_to_update = true;
                }

                if (needs_to_be_disabled)
                    style::endDisabled();

                ImGui::SameLine();
                ImGui::ProgressBar(progress);
            }

            if (ImGui::CollapsingHeader("ImageProduct Advanced"))
            {
                if (ImGui::BeginTabBar("###imageproducttuning", ImGuiTabBarFlags_FittingPolicyScroll))
                {
                    for (auto &ch : product->images)
                    {
                        std::string id = "Channel " + ch.channel_name;
                        if (ImGui::BeginTabItem(id.c_str()))
                        {
                            ch.ch_transform.render();
                            ImGui::EndTabItem();
                        }
                    }
                    ImGui::EndTabBar();
                }
            }

            /// TODOREWORK UPDATE
            if (needs_to_update)
            {
                asyncProcess();
                needs_to_update = false;
            }

            img_handler.drawMenu();
        }

        void ImageProductHandler::setConfig(nlohmann::json p)
        {
            if (p.contains("equation"))
            {
                equation = p["equation"];
                channel_selection_curr_id = -1;
            }
            else if (p.contains("channel"))
            {
                for (int i = 0; i < product->images.size(); i++)
                    if (product->images[i].channel_name == p["channel"].get<std::string>())
                        channel_selection_curr_id = i;
            }

            if (p.contains("image"))
                img_handler.setConfig(p["image"]);
        }

        nlohmann::json ImageProductHandler::getConfig()
        {
            nlohmann::json p;

            if (channel_selection_curr_id == -1)
                p["equation"] = equation;
            else
                p["channel"] = product->images[channel_selection_curr_id].channel_name;

            p["image"] = img_handler.getConfig();

            return p;
        }

        void ImageProductHandler::do_process()
        {
            try
            {
                if (channel_selection_curr_id == -1)
                {
                    auto img = products::generate_equation_product_composite(product, equation, &progress);
                    img_handler.updateImage(img);
                }
                else
                {
                    auto img = product->images[channel_selection_curr_id].image; // TODOREWORK MAKE FUNCTION TO GET SINGLE CHANNEL
                    img_handler.updateImage(img);
                }
            }
            catch (std::exception &e)
            {
                logger->error("Could not process image! %s", e.what());
            }
        }

        void ImageProductHandler::drawMenuBar()
        {
            img_handler.drawMenuBar();
        }

        void ImageProductHandler::drawContents(ImVec2 win_size)
        {
            img_handler.drawContents(win_size);
        }
    }
}
