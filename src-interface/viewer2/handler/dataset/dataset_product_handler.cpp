#include "dataset_product_handler.h"
#include "logger.h"

namespace satdump
{
    namespace viewer
    {
        DatasetProductHandler::DatasetProductHandler()
        {
            editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
        }

        DatasetProductHandler::~DatasetProductHandler()
        {
        }

#if 0
         class BlockNode : public ImFlow::BaseNode
        {
        public:
            BlockNode()
            {
                setTitle("BlockSource");
                setStyle(ImFlow::NodeStyle::green());

                addIN<std::string>("In", "!!invalid!!", ImFlow::ConnectionFilter::SameType());

                auto fun = [this]()
                {
                    return (v == 0 ? "Firstin" : getInVal<std::string>("In")) + "\n" + "BlockNode" + std::to_string(v);
                };

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
#endif

        class BlockSourceNode : public ImFlow::BaseNode
        {
        public:
            BlockSourceNode()
            {
                setTitle("BlockSource");
                setStyle(ImFlow::NodeStyle::green());

                auto fun = [this]()
                {
                    return "val = 6\n";
                };

                addOUT<std::string>("Out", ImFlow::PinStyle::brown())->behaviour(fun);
            }

            ~BlockSourceNode()
            {
            }

            void draw() override
            {
                ImGui::Text("Something");
            }

            std::string eval()
            {
                logger->info("OutSize %d", getOuts().size());
            }
        };

        class BlockSinkNode : public ImFlow::BaseNode
        {
        public:
            std::function<std::string()> fun = [this]()
            {
                return getInVal<std::string>("In") + "print(val)\n";
            };

            BlockSinkNode()
            {
                setTitle("BlockSink");
                setStyle(ImFlow::NodeStyle::green());

                addIN<std::string>("In", "!!invalid!!", ImFlow::ConnectionFilter::SameType());

                //   addOUT<std::string>("Out", ImFlow::PinStyle::brown())->behaviour(fun);
            }

            ~BlockSinkNode()
            {
            }

            void draw() override
            {
                ImGui::Text("Something");
            }

            std::string eval()
            {
                logger->info("OutSize %d", getOuts().size());
                return fun();
            }
        };

        class BlockMidNode : public ImFlow::BaseNode
        {
        public:
            std::function<std::string()> fun = [this]()
            {
                return getInVal<std::string>("In") + "val = val + 1\n";
            };

            BlockMidNode()
            {
                setTitle("BlockMid");
                setStyle(ImFlow::NodeStyle::green());

                addIN<std::string>("In", "!!invalid!!", ImFlow::ConnectionFilter::SameType());

                addOUT<std::string>("Out", ImFlow::PinStyle::brown())->behaviour(fun);
            }

            ~BlockMidNode()
            {
            }

            void draw() override
            {
                ImGui::Text("Something");
            }

            std::string eval()
            {
                logger->info("OutSize %d", getOuts().size());
                return fun();
            }
        };

        std::shared_ptr<BlockSinkNode> final_node = 0;

        void DatasetProductHandler::drawMenu()
        {
            if (selected_tab == 0)
            {
            }
            else if (selected_tab == 1)
            {
                if (ImGui::CollapsingHeader("Flowgraph"))
                {
                    if (ImGui::Button("Add BlockSource"))
                        grid.addNode<BlockSourceNode>({0, 0});
                    if (ImGui::Button("Add BlockSink"))
                        final_node = grid.addNode<BlockSinkNode>({0, 0});
                    if (ImGui::Button("Add BlockMid"))
                        grid.addNode<BlockMidNode>({0, 0});

                    if (ImGui::Button("Eval"))
                    {
                        editor.SetText(final_node->eval());
                    }
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
                grid.update();
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