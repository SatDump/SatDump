#include "dsp_flowgraph_handler.h"
#include "common/widgets/menuitem_tooltip.h"
#include "core/backend.h"

#include "dsp/flowgraph/dsp_flowgraph_register.h"
#include "imgui/imgui.h"
#include "imgui/imgui_flags.h"
#include "imgui/imnodes/imnodes.h"
#include "logger.h"

#include "nlohmann/json_utils.h"
#include <complex.h>
#include <fstream>

namespace satdump
{
    namespace handlers
    {
        DSPFlowGraphHandler::DSPFlowGraphHandler(std::string file)
        {
            handler_tree_icon = u8"\uf92f";

            ndsp::registerNodesInFlowgraph(flowgraph);

            if (file != "")
                flowgraph.setJSON(loadCborFile(file));
            current_file = file;
        }

        DSPFlowGraphHandler::~DSPFlowGraphHandler()
        {
            if (flow_thread.joinable())
                flow_thread.join();

            if (imnode_ctx != nullptr)
                ImNodes::DestroyContext(imnode_ctx);
        }

        void DSPFlowGraphHandler::drawMenu()
        {
            bool running = flowgraph.is_running;

            if (ImGui::CollapsingHeader("Variables"))
            {
                if (running)
                    style::beginDisabled();

                for (auto &v : flowgraph.variables)
                {
                    ImGui::Text("%s", v.first.c_str());
                    ImGui::SameLine();
                    ImGui::InputDouble(std::string("##" + v.first).c_str(), &v.second);
                }

                ImGui::InputText("##addvarname", &to_add_var_name);
                ImGui::SameLine();
                if (ImGui::Button("Add##addnewvariable") && to_add_var_name.size())
                {
                    flowgraph.variables.emplace(to_add_var_name, 0);
                    to_add_var_name.clear();
                }

                if (running)
                    style::endDisabled();
            }
        }

        void DSPFlowGraphHandler::drawMenuBar()
        {
            bool running = flowgraph.is_running;

            if (running)
            {
                if (widgets::MenuItemTooltip("\ufc62", "Stop flowgraph"))
                    flowgraph.stop();
            }
            else
            {
                if (widgets::MenuItemTooltip("\uf909", "Start flowgraph"))
                {
                    if (flow_thread.joinable())
                        flow_thread.join();
                    auto fun = [&]() { flowgraph.run(); };
                    flow_thread = std::thread(fun);
                }
            }

            bool is_save_as = false;
            if (widgets::MenuItemTooltip("\ueb4b", "Save file", NULL, false, current_file != "") || //
                (is_save_as = widgets::MenuItemTooltip("\ueb4a", "Save file as")) ||                //
                (ImGui::IsKeyDown(ImGuiKey_ReservedForModCtrl) && ImGui::IsKeyPressed(ImGuiKey_S)))
            {
                auto fun = [this, is_save_as]()
                {
                    std::string save_at = is_save_as ? backend::saveFileDialog({{"SatDump DSP Flowgraph", "satdump_dsp_flowgraph"}}, "", "flowgraph.satdump_dsp_flowgraph") : current_file;

                    logger->info("Saving flowgraph : " + save_at);
                    if (save_at == "")
                        return;

                    saveCborFile(save_at, flowgraph.getJSON());
                    current_file = save_at;
                };
                tq.push(fun);
            }

            if (widgets::MenuItemTooltip("\uea7f", "Load a file"))
            {
                auto fun = [this]()
                {
                    std::string load_at = backend::selectFileDialog({{"SatDump DSP Flowgraph", "satdump_dsp_flowgraph"}}, "");

                    logger->info("Loading flowgraph : " + load_at);
                    if (load_at == "")
                        return;

                    flowgraph.setJSON(loadCborFile(load_at));
                };
                tq.push(fun);
            }

#if 0
            if (ImGui::MenuItem("To Clipboard"))
            {
                std::string json = flowgraph.getJSON().dump(4);
                ImGui::SetClipboardText(json.c_str());
            }

            if (ImGui::MenuItem("From Clipboard"))
            {
                nlohmann::json v = nlohmann::json::parse(ImGui::GetClipboardText());
                flowgraph.setJSON(v);
            }
#endif
        }

        void DSPFlowGraphHandler::drawContents(ImVec2 win_size)
        {
            if (imnode_ctx == nullptr)
                imnode_ctx = ImNodes::CreateContext();
            ImNodes::SetCurrentContext(imnode_ctx);

            ctx.config().color = ImGui::GetColorU32(ImGuiCol_WindowBg);

            ctx.begin();

            float f = 15;
            ImGui::SetNextWindowPos({-f / ctx.scale(), -f / ctx.scale()});
            ImGui::SetNextWindowSize({(float)(win_size.x / ctx.scale()) + 2 * (f / ctx.scale()), (float)(win_size.y / ctx.scale()) + 2 * (f / ctx.scale())});
            ImGui::Begin("SatDump UI", nullptr, NOWINDOW_FLAGS | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse);
            flowgraph.render();
            ImGui::End();

            ctx.end();
        }
    } // namespace handlers
} // namespace satdump