#include "dsp_flowgraph_handler.h"
#include "core/backend.h"

#include "dsp/flowgraph/dsp_flowgraph_register.h"
#include "imgui/imgui.h"
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
            // TODOREWORK
            if (ImNodes::GetCurrentContext() == nullptr)
                ImNodes::CreateContext();

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
        }

        void DSPFlowGraphHandler::drawMenu()
        {
            bool running = flowgraph.is_running;

            if (ImGui::CollapsingHeader("Flowgraph"))
            {
                if (running)
                {
                    if (ImGui::Button("Stop"))
                        flowgraph.stop();
                }
                else
                {
                    if (ImGui::Button("Start"))
                    {
                        if (flow_thread.joinable())
                            flow_thread.join();
                        auto fun = [&]() { flowgraph.run(); };
                        flow_thread = std::thread(fun);
                    }
                }

                if (running)
                    style::beginDisabled();

#if 0
                auto pos = ImGui::GetCursorPos();
                ImGui::Button("##test", {50, 50});
                pos.x += 10;
                ImGui::GetWindowDrawList()                        //
                    ->AddTriangleFilled({pos.x + 10, pos.y + 43}, //
                                        {pos.x + 10, pos.y + 77}, //
                                        {pos.x + 40, pos.y + 60}, //
                                        ImColor(255, 255, 255, 255));
#endif

                //            needs_to_update |= TODO; // TODOREWORK move in top drawMenu?

                if (running)
                    style::endDisabled();
            }

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
            if (ImGui::MenuItem("Save"))
            {
                auto fun = [this]()
                {
                    std::string save_at = current_file == "" ? backend::saveFileDialog({{"SatDump DSP Flowgraph", "satdump_dsp_flowgraph"}}, "", "flowgraph.satdump_dsp_flowgraph") : current_file;

                    logger->info("Saving flowgraph : " + save_at);
                    if (save_at == "")
                        return;

                    saveCborFile(save_at, flowgraph.getJSON());
                };
                tq.push(fun);
            }

            if (ImGui::MenuItem("Load"))
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
        }

        void DSPFlowGraphHandler::drawContents(ImVec2 win_size) { flowgraph.render(); }
    } // namespace handlers
} // namespace satdump