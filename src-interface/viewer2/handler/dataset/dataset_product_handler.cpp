#include "dataset_product_handler.h"
#include "logger.h"

#include "imgui/imnodes/imnodes.h"

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
        }

        DatasetProductHandler::~DatasetProductHandler()
        {
        }

        namespace
        {
            class FlowchartNode
            {
            };

            class FlowchartManager
            {
                std::vector<FlowchartNode> nodes;
            };

            FlowchartManager managerTest;
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
                    /* if (ImGui::Button("Add BlockSource"))
                         grid.addNode<BlockSourceNode>({0, 0});
                     if (ImGui::Button("Add BlockSink"))
                         final_node = grid.addNode<BlockSinkNode>({0, 0});
                     if (ImGui::Button("Add BlockMid"))
                         grid.addNode<BlockMidNode>({0, 0});

                     if (ImGui::Button("Eval"))
                     {
                         editor.SetText(final_node->eval());
                     }*/
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

                //
                const int hardcoded_node_id = 1;
                ImNodes::BeginNodeEditor();

                ImNodes::BeginNode(hardcoded_node_id);

                ImNodes::BeginNodeTitleBar();
                ImGui::Text("output node");
                ImNodes::EndNodeTitleBar();

                ImNodes::BeginOutputAttribute(4, 1);
                ImGui::Text("Out1");
                ImNodes::EndOutputAttribute();

                ImNodes::EndNode();

                ImNodes::EndNodeEditor();
                //

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
    }
}