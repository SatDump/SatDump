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

        class BlockNode : public ImFlow::BaseNode
        {

        public:
            BlockNode()
            {
                setTitle("BlockNode");
                setStyle(ImFlow::NodeStyle::green());
                //    addIN<std::shared_ptr<NaFiFo2>>("In", 0, ImFlow::ConnectionFilter::SameType());
                // if (blk->outputs.size() > 0)
                //    addOUT<std::shared_ptr<NaFiFo2>>("Out", ImFlow::PinStyle::brown())->behaviour([this]()
                //                                                                                  { return blk->get_output(0); });
            }

            ~BlockNode()
            {
            }

            void draw() override
            {
                ImGui::Text("Something");
            }
        };

        void DatasetProductHandler::drawMenu()
        {
            if (selected_tab == 0)
            {
            }
            else if (selected_tab == 1)
            {
                if (ImGui::CollapsingHeader("Flowgraph"))
                {
                    if (ImGui::Button("Add Block"))
                    {
                        grid.addNode<BlockNode>({0, 0});
                    }
                }
            }
            else if (selected_tab == 2)
            {
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
                grid.update();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Code"))
            {
                selected_tab = 2;

                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
}
