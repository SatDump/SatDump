#include "dataset_product_handler.h"
#include "logger.h"

#include "imgui/imnodes/imnodes.h"

// TODOREWORK
#include "lua_processor.h"

#include "flowgraph/imageproduct_node.h"
#include "flowgraph/image_nodes.h"
#include "flowgraph/datasetproc_node.h"

namespace satdump
{
    namespace viewer
    {
        DatasetProductHandler::DatasetProductHandler()
        {
            editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());

            // TODOREWORK
            if (ImNodes::GetCurrentContext() == nullptr)
                ImNodes::CreateContext();

            flowgraph.node_internal_registry.emplace("image_product_source", []()
                                                     { return std::make_shared<ImageProductSource_Node>(); });
            flowgraph.node_internal_registry.emplace("image_product_equation", []()
                                                     { return std::make_shared<ImageProductEquation_Node>(); });
            flowgraph.node_internal_registry.emplace("image_sink", []()
                                                     { return std::make_shared<ImageSink_Node>(); });
            flowgraph.node_internal_registry.emplace("image_get_proj", []()
                                                     { return std::make_shared<ImageGetProj_Node>(); });
            flowgraph.node_internal_registry.emplace("image_reproj", []()
                                                     { return std::make_shared<ImageReproj_Node>(); });
            flowgraph.node_internal_registry.emplace("image_equation", []()
                                                     { return std::make_shared<ImageEquation_Node>(); });
            flowgraph.node_internal_registry.emplace("image_handler_sink", [this]()
                                                     { return std::make_shared<ImageHandlerSink_Node>(this); });
            flowgraph.node_internal_registry.emplace("image_equalize", []()
                                                     { return std::make_shared<ImageEqualize_Node>(); });
            flowgraph.node_internal_registry.emplace("image_source", []()
                                                     { return std::make_shared<ImageSource_Node>(); });
        }

        DatasetProductHandler::~DatasetProductHandler()
        {
        }

        void DatasetProductHandler::drawMenu()
        {
            if (selected_tab == 0)
            {
            }
            else if (selected_tab == 1)
            {
                if (ImGui::CollapsingHeader("Flowgraph"))
                {
                    if (ImGui::Button("Get JSON"))
                    {
                        std::string str = flowgraph.getJSON().dump(4);
                        ImGui::SetClipboardText(str.c_str());
                    }

                    if (ImGui::Button("Set JSON"))
                    {
                        auto str = ImGui::GetClipboardText();
                        flowgraph.setJSON(nlohmann::json::parse(str));
                    }

                    if (ImGui::Button("Run"))
                        flowgraph.run();
                }
            }
            else if (selected_tab == 2)
            {
                if (ImGui::Button("Run"))
                    run_lua();
            }
        }

        void DatasetProductHandler::drawContents(ImVec2 win_size)
        {
            ImGui::BeginTabBar("##datasetproducttabbar");
            if (ImGui::BeginTabItem("Presets"))
            {
                selected_tab = 0;
                if (ImGui::BeginListBox("##pipelineslistbox"))
                {
                    bool is_selected = false;
                    ImGui::Selectable("Full Swath KMSS 321", &is_selected);
                    ImGui::Selectable("Full Swath KMSS 231", &is_selected);
                    ImGui::EndListBox();
                }
                ImGui::EndTabItem();

                if (ImGui::BeginTable("##pipelinesmainoptions", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
                {
                    ImGui::TableSetupColumn("##pipelinesmaincolumn1", ImGuiTableColumnFlags_None);
                    ImGui::TableSetupColumn("##pipelinesmaincolumn2", ImGuiTableColumnFlags_None);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Equation");
                    ImGui::TableSetColumnIndex(1);
                    char t[100] = "ch3, ch2, ch1";
                    ImGui::InputText("##equation", t, 100);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("Some Other Param");
                    ImGui::TableSetColumnIndex(1);
                    bool check = true;
                    ImGui::Checkbox("###checkboxtest", &check);

                    ImGui::EndTable();
                }

                ImGui::Button("Generate");
            }
            if (ImGui::BeginTabItem("Flowgraph"))
            {
                selected_tab = 1;
                // grid.update();
                flowgraph.render();

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Code"))
            {
                selected_tab = 2;
                editor.Render("Test");
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        void DatasetProductHandler::run_lua()
        {
            // TODOREWORK
            nlohmann::json t;
            t["lua"] = editor.GetText();
            DatasetProductProcessor *proc = new Lua_DatasetProductProcessor(dataset_handler, this, t);
            proc->process();
            delete proc;
        }
    }
}