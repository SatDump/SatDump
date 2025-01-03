#include "dataset_product_handler.h"
#include "logger.h"

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

        int vvvv = 0; // TODOREWORK DELETE
        class BlockNode : public ImFlow::BaseNode
        {
        public:
            int v = 0;
            BlockNode()
            {
                v = vvvv++;
                setTitle("BlockNode" + std::to_string(v));
                setStyle(ImFlow::NodeStyle::green());

                if (v != 0)
                    addIN<std::string>("In", "!!invalid!!", ImFlow::ConnectionFilter::SameType());

                auto fun = [this]()
                {
                    return (v == 0 ? "Firstin" : getInVal<std::string>("In")) + "\n" + "BlockNode" + std::to_string(v);
                };

                if (v != 3)
                    addOUT<std::string>("Out", ImFlow::PinStyle::brown())->behaviour(fun);
            }

            ~BlockNode()
            {
            }

            void draw() override
            {
                ImGui::Text("Something");
            }

            void eval()
            {
                logger->info("OutSize %d", getOuts().size());
                if (getOuts().size() == 0)
                {
                    std::string vl = getInVal<std::string>("In");
                    printf("finalout \n%s\n", vl.c_str());
                }
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

                    if (ImGui::Button("Eval"))
                    {
                        for (auto &n : grid.getNodes())
                            ((BlockNode *)(n.second.get()))->eval(); // printf("Node %s\n", n.second->getName().c_str());
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
