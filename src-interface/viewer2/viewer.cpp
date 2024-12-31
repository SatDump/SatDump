#include "viewer.h"
// #include "image_handler.h"
// #include "radiation_handler.h"
// #include "scatterometer_handler.h"
// #include "core/config.h"
// #include "products/dataset.h"
#include "common/utils.h"
// #include "resources.h"
#include "main_ui.h"

#include "dummy_handler.h"
#include "image_product_handler.h" // TODO CLEAN

namespace satdump
{
    namespace viewer
    {
        ViewerApplication::ViewerApplication()
            : Application("viewer")
        {
            master_handler = std::make_shared<DummyHandler>();
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

                if (ImGui::Button("Load KMSS"))
                {
                    std::shared_ptr<ProductHandler> prod_h = std::make_shared<ImageProductHandler>();
                    prod_h->product = products::loadProduct("/home/alan/Downloads/SatDump_NEWPRODS/KMSS_24/KMSS_MSU100_1/product.cbor");
                    prod_h->init();
                    master_handler->addSubHandler(prod_h);
                }
                else if (ImGui::Button("Load Sterna"))
                {
                    std::shared_ptr<ProductHandler> prod_h = std::make_shared<ImageProductHandler>();
                    prod_h->product = products::loadProduct("/home/alan/Downloads/SatDump_NEWPRODS/aws_pfm_cadu/STERNA_Dump/product.cbor");
                    prod_h->init();
                    master_handler->addSubHandler(prod_h);
                }

                if (curr_handler)
                    curr_handler->drawMenu();
            }
        }

        void ViewerApplication::drawContent()
        {
        }

        void ViewerApplication::drawUI()
        {
            ImVec2 viewer_size = ImGui::GetContentRegionAvail();

            if (ImGui::BeginTable("##wiever_table", 2, ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp))
            {
                ImGui::TableSetupColumn("##panel_v", ImGuiTableColumnFlags_None, viewer_size.x * panel_ratio);
                ImGui::TableSetupColumn("##view", ImGuiTableColumnFlags_None, viewer_size.x * (1.0f - panel_ratio));
                ImGui::TableNextColumn();

                float left_width = ImGui::GetColumnWidth(0);
                float right_width = viewer_size.x - left_width;
                if (left_width != last_width && last_width != -1)
                    panel_ratio = left_width / viewer_size.x;
                last_width = left_width;

                ImGui::BeginChild("ViewerChildPanel", {left_width, float(viewer_size.y - 10)});
                drawPanel();
                ImGui::EndChild();

                ImGui::TableNextColumn();
                ImGui::BeginGroup();

                if (curr_handler)
                    curr_handler->drawContents({float(right_width - 4), float(viewer_size.y)});

                ImGui::EndGroup();
                ImGui::EndTable();
            }
        }
    }
};
