#include "viewer.h"
// #include "image_handler.h"
// #include "radiation_handler.h"
// #include "scatterometer_handler.h"
// #include "core/config.h"
// #include "products/dataset.h"
#include "common/utils.h"
// #include "resources.h"
#include "main_ui.h"

#include "handler/dummy_handler.h"
#include "handler/product/image_product_handler.h" // TODO CLEAN
#include "handler/dataset/dataset_handler.h"

namespace satdump
{
    namespace viewer
    {
        ViewerApplication::ViewerApplication()
            : Application("viewer")
        {
            master_handler = std::make_shared<DummyHandler>("MasterHandlerViewer");
        }

        ViewerApplication::~ViewerApplication()
        {
        }

        void ViewerApplication::drawPanel()
        {
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 2);

            if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::BeginTable("##masterhandlertable", 1, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders))
                {
                    ImGui::TableSetupColumn("##masterhandlertable_col", ImGuiTableColumnFlags_None);
                    ImGui::TableNextColumn();

                    master_handler->drawTreeMenu(curr_handler);

                    ImGui::EndTable();
                }

                if (curr_handler)
                    curr_handler->drawMenu();
            }
        }

        void ViewerApplication::drawMenuBar()
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    ImGui::MenuItem("Open Dataset");
                    ImGui::MenuItem("Open Product");
                    ImGui::MenuItem("Open Image");

                    if (ImGui::BeginMenu("Hardcoded"))
                    {
                        if (ImGui::MenuItem("Load KMSS"))
                        {
                            std::shared_ptr<ProductHandler> prod_h = std::make_shared<ImageProductHandler>(products::loadProduct("/home/alan/Downloads/SatDump_NEWPRODS/KMSS_24/KMSS_MSU100_1/product.cbor"));
                            master_handler->addSubHandler(prod_h);
                        }
                        if (ImGui::MenuItem("Load Sterna"))
                        {
                            std::shared_ptr<ProductHandler> prod_h = std::make_shared<ImageProductHandler>(products::loadProduct("/home/alan/Downloads/SatDump_NEWPRODS/aws_pfm_cadu/STERNA_Dump/product.cbor"));
                            master_handler->addSubHandler(prod_h);
                        }
                        if (ImGui::MenuItem("Dataset"))
                        {
                            std::shared_ptr<DatasetHandler> dat_h = std::make_shared<DatasetHandler>();
                            {
                                std::shared_ptr<ProductHandler> prod_h = std::make_shared<ImageProductHandler>(products::loadProduct("/home/alan/Downloads/SatDump_NEWPRODS/KMSS_24/KMSS_MSU100_1/product.cbor"));
                                dat_h->instrument_products->addSubHandler(prod_h);
                            }
                            {
                                std::shared_ptr<ProductHandler> prod_h = std::make_shared<ImageProductHandler>(products::loadProduct("/home/alan/Downloads/SatDump_NEWPRODS/KMSS_24/KMSS_MSU100_2/product.cbor"));
                                dat_h->instrument_products->addSubHandler(prod_h);
                            }
                            {
                                std::shared_ptr<ProductHandler> prod_h = std::make_shared<ImageProductHandler>(products::loadProduct("/home/alan/Downloads/SatDump_NEWPRODS/aws_pfm_cadu/STERNA_Dump/product.cbor"));
                                dat_h->instrument_products->addSubHandler(prod_h);
                            }
                            master_handler->addSubHandler(dat_h);
                        }
                        if (ImGui::MenuItem("Load MSUGS"))
                        {
                            std::shared_ptr<ProductHandler> prod_h = std::make_shared<ImageProductHandler>(products::loadProduct("/home/alan/Downloads/20241231_132953_ARKTIKA-M 2_dat/MSUGS_VIS1/product.cbor"));
                            master_handler->addSubHandler(prod_h);
                        }
                        if (ImGui::MenuItem("Load MSUGS 2"))
                        {
                            std::shared_ptr<ProductHandler> prod_h = std::make_shared<ImageProductHandler>(products::loadProduct("/home/alan/Downloads/20241231_154404_ARKTIKA-M 2_dat/MSUGS_VIS1/product.cbor"));
                            master_handler->addSubHandler(prod_h);
                        }
                        ImGui::EndMenu();
                    }

                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Handler"))
                {
                    if (ImGui::BeginMenu("Add"))
                    {
                        ImGui::MenuItem("Dataset");
                        ImGui::MenuItem("Projection");
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenu();
                }

                if (curr_handler)
                {
                    if (ImGui::BeginMenu("Current"))
                    {
                        curr_handler->drawMenuBar();
                        ImGui::EndMenu();
                    }
                }

                ImGui::EndMenuBar();
            }
        }

        void ViewerApplication::drawContents()
        {
            ImGui::Text("No handler selected!");
        }

        void ViewerApplication::drawUI()
        {
            ImVec2 viewer_size = ImGui::GetContentRegionAvail();

            if (ImGui::BeginTable("##wiever_table", 2, /*ImGuiTableFlags_NoBordersInBodyUntilResize |*/ ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV))
            {
                ImGui::TableSetupColumn("##panel_v", ImGuiTableColumnFlags_None, viewer_size.x * panel_ratio);
                ImGui::TableSetupColumn("##view", ImGuiTableColumnFlags_None, viewer_size.x * (1.0f - panel_ratio));
                ImGui::TableNextColumn();

                float left_width = ImGui::GetColumnWidth(0);
                float right_width = viewer_size.x - left_width;
                if (left_width != last_width && last_width != -1)
                    panel_ratio = left_width / viewer_size.x;
                last_width = left_width;

                ImGui::BeginChild("ViewerChildPanel", {left_width, float(viewer_size.y - 10)}, false, ImGuiWindowFlags_MenuBar);
                drawMenuBar();
                drawPanel();
                ImGui::EndChild();

                ImGui::TableNextColumn();
                ImGui::BeginGroup();

                if (curr_handler)
                    curr_handler->drawContents({float(right_width - 4), float(viewer_size.y)});
                else
                    drawContents();

                ImGui::EndGroup();
                ImGui::EndTable();
            }
        }
    }
};
