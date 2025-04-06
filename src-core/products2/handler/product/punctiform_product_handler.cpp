#include "punctiform_product_handler.h"
#include "nlohmann/json_utils.h"

#include "imgui/imgui_stdlib.h"
#include "imgui/implot/implot.h" // TODOREWORK

namespace satdump
{
    namespace viewer
    {
        PunctiformProductHandler::PunctiformProductHandler(std::shared_ptr<products::Product> p)
            : ProductHandler(p)
        {
            handler_tree_icon = "\uf71e";

            product = (products::PunctiformProduct *)ProductHandler::product.get();

            tryApplyDefaultPreset();
        }

        PunctiformProductHandler::~PunctiformProductHandler()
        {
        }

        void PunctiformProductHandler::drawMenu()
        {
            bool needs_to_be_disabled = is_processing;

            if (ImGui::CollapsingHeader("Channels", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (needs_to_be_disabled)
                    style::beginDisabled();

                if (needs_to_be_disabled)
                    style::endDisabled();
            }

            //            needs_to_update |= renderPresetMenu(); // TODOREWORK move in top drawMenu?
        }

        void PunctiformProductHandler::setConfig(nlohmann::json p)
        {
            // if (p.contains("expression"))
            // {
            //     expression = p["expression"];
            //     channel_selection_curr_id = -1;
            // }
        }

        nlohmann::json PunctiformProductHandler::getConfig()
        {
            nlohmann::json p;

            // p["channel_calibrated"] = channel_calibrated;

            return p;
        }

        void PunctiformProductHandler::do_process()
        {
            try
            {
            }
            catch (std::exception &e)
            {
                logger->error("Could not process image! %s", e.what());
            }
        }

        void PunctiformProductHandler::saveResult(std::string directory)
        {
            // TODOREWORK
        }

        void PunctiformProductHandler::drawMenuBar()
        {
        }

        void PunctiformProductHandler::drawContents(ImVec2 win_size)
        {
            ImPlot::BeginPlot("PlotHandler");

            if (product->data.size() >= 3)
            {
                logger->info(product->data[0].timestamps.size());
                std::vector<double> t;
                for (int i = 0; i < product->data[0].timestamps.size(); i++)
                    t.push_back(i);
                ImPlot::PlotLine("Data 1", t.data(), product->data[0].data.data(), product->data[0].timestamps.size());
            }

            ImPlot::EndPlot();
        }
    }
}
