#include "dataset_product_handler.h"

namespace satdump
{
    namespace viewer
    {
        void DatasetProductHandler::init()
        {
        }

        DatasetProductHandler::~DatasetProductHandler()
        {
        }

        void DatasetProductHandler::drawMenu()
        {
        }

        void DatasetProductHandler::drawContents(ImVec2 win_size)
        {
            ImGui::BeginTabBar("##datasetproducttabbar");
            if (ImGui::BeginTabItem("Presets"))
            {
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
                grid.update();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Code"))
            {

                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
}
