#include "punctiform_product_handler.h"
#include "nlohmann/json_utils.h"

#include "imgui/imgui_stdlib.h"
#include "imgui/implot/implot.h" // TODOREWORK

#include "products2/punctiform/product_dotmap.h"

namespace satdump
{
    namespace viewer
    {
        PunctiformProductHandler::PunctiformProductHandler(std::shared_ptr<products::Product> p)
            : ProductHandler(p)
        {
            handler_tree_icon = "\uf71e";

            product = (products::PunctiformProduct *)ProductHandler::product.get();
            if (product->data.size())
                selected_channel = product->data[0].channel_name;

            tryApplyDefaultPreset();
        }

        PunctiformProductHandler::~PunctiformProductHandler()
        {
        }

        void PunctiformProductHandler::drawMenu()
        {
            bool needs_to_be_disabled = is_processing;
            bool needs_to_update = false;

            if (ImGui::CollapsingHeader("Channels", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (needs_to_be_disabled)
                    style::beginDisabled();

                if (ImGui::RadioButton("Graph", current_mode == MODE_GRAPH))
                {
                    current_mode = MODE_GRAPH;
                    needs_to_update = true;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Dot Map", current_mode == MODE_DOTMAP))
                {
                    current_mode = MODE_DOTMAP;
                    needs_to_update = true;
                }

                if (ImGui::BeginCombo("Channel##selectechannel", selected_channel.c_str()))
                {
                    for (auto &p : product->data)
                    {
                        if (ImGui::Selectable(p.channel_name.c_str(), selected_channel == p.channel_name))
                        {
                            selected_channel = p.channel_name;
                            needs_to_update = true;
                        }
                    }
                    ImGui::EndCombo();
                }

                if (current_mode == MODE_DOTMAP)
                {
                    ImGui::InputDouble("Range Min", &range_min);
                    ImGui::InputDouble("Range Max", &range_max);
                }

                needs_to_update |= ImGui::Button("Update");

                if (needs_to_be_disabled)
                    style::endDisabled();
            }

            needs_to_update |= renderPresetMenu(); // TODOREWORK move in top drawMenu?

            if (current_mode == MODE_DOTMAP)
                img_handler.drawMenu();

            if (needs_to_update)
                asyncProcess();
        }

        void PunctiformProductHandler::setConfig(nlohmann::json p)
        {
            if (p.contains("mode"))
            {
                if (p["mode"] == "graph")
                    current_mode = MODE_GRAPH;
                else if (p["mode"] == "dotmap")
                    current_mode = MODE_DOTMAP;
            }

            selected_channel = getValueOrDefault(p["selected_channel"], selected_channel);
            range_min = getValueOrDefault(p["range_min"], range_min);
            range_max = getValueOrDefault(p["range_max"], range_max);
        }

        nlohmann::json PunctiformProductHandler::getConfig()
        {
            nlohmann::json p;

            if (current_mode == MODE_GRAPH)
                p["mode"] = "graph";
            else if (current_mode == MODE_DOTMAP)
                p["mode"] = "dotmap";

            p["channel"] = selected_channel;
            p["range_min"] = range_min = 0;
            p["range_max"] = range_max = 10;

            return p;
        }

        void PunctiformProductHandler::do_process()
        {
            try
            {
                if (current_mode == MODE_DOTMAP)
                {
                    auto img = products::generate_dotmap_product_image(product, selected_channel, -1, -1, 5, true, true, range_min, range_max);
                    img_handler.updateImage(img);
                }
            }
            catch (std::exception &e)
            {
                logger->error("Could not process punctiform data! %s", e.what());
            }
        }

        void PunctiformProductHandler::saveResult(std::string directory)
        {
            // TODOREWORK
        }

        void PunctiformProductHandler::drawMenuBar()
        {
            if (current_mode == MODE_DOTMAP)
                img_handler.drawMenuBar();
        }

        void PunctiformProductHandler::drawContents(ImVec2 win_size)
        {
            if (current_mode == MODE_GRAPH)
            {
                ImPlot::BeginPlot("PlotHandler", {win_size.x, win_size.y});

                for (auto &ch : product->data)
                {
                    if (ch.channel_name == selected_channel)
                    {
                        std::vector<double> t;
                        for (int i = 0; i < ch.timestamps.size(); i++)
                            t.push_back(i);
                        ImPlot::PlotLine(std::string("Channel " + ch.channel_name).c_str(), t.data(), ch.data.data(), ch.timestamps.size());
                    }
                }

                ImPlot::EndPlot();
            }
            else if (current_mode == MODE_DOTMAP)
            {
                img_handler.drawContents(win_size);
            }
        }
    }
}
