#include "viewer.h"
#include "image_handler.h"
#include "radiation_handler.h"
#include "scatterometer_handler.h"
#include "core/config.h"
#include "products/dataset.h"
#include "common/utils.h"
#include "error.h"

void SelectableColor(ImU32 color) // funkcja pozwalająca na pokolorowanie komówki w tabelii na wybrany kolor w RGBA
{
    ImVec2 p_min = ImGui::GetItemRectMin();
    ImVec2 p_max = ImGui::GetItemRectMax();
    ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, color);
}

namespace satdump
{
    ViewerApplication::ViewerApplication()
        : Application("viewer")
    {
        if (config::main_cfg["user"].contains("viewer_state"))
        {
            if (config::main_cfg["user"]["viewer_state"].contains("panel_ratio"))
                panel_ratio = config::main_cfg["user"]["viewer_state"]["panel_ratio"].get<float>();

            if (config::main_cfg["user"]["viewer_state"].contains("projections"))
                deserialize_projections_config(config::main_cfg["user"]["viewer_state"]["projections"]);
        }

        // pro.load("/home/alan/Documents/SatDump_ReWork/build/metop_ahrpt_new/AVHRR/product.cbor");
        //  pro.load("/home/alan/Documents/SatDump_ReWork/build/aqua_test_new/MODIS/product.cbor");

        // loadDatasetInViewer("/home/alan/Documents/SatDump_ReWork/build/metop_ahrpt_new/dataset.json");
        // loadDatasetInViewer("/home/alan/Documents/SatDump_ReWork/build/metop_idk_damnit/dataset.json");

        // loadDatasetInViewer("/tmp/hirs/dataset.json");

        // loadDatasetInViewer("/home/zbyszek/Downloads/metopC_15-04_1125/dataset.json");

        // loadProductsInViewer("/home/alan/Documents/SatDump_ReWork/build/noaa_mhs_test/AMSU/product.cbor", "NOAA-19 HRPT");
        // loadProductsInViewer("/home/alan/Documents/SatDump_ReWork/build/metop_ahrpt_new/AVHRR/product.cbor", "MetOp-B AHRPT");
        // loadProductsInViewer("/home/alan/Documents/SatDump_ReWork/build/noaa_mhs_test/MHS/product.cbor", "NOAA-19 HRPT");
        // loadProductsInViewer("/home/alan/Documents/SatDump_ReWork/build/noaa_mhs_test_gac/MHS/product.cbor", "NOAA-19 GAC");
        // loadProductsInViewer("/home/alan/Documents/SatDump_ReWork/build/noaa_mhs_test_gac/HIRS/product.cbor", "NOAA-19 GAC");
        // loadProductsInViewer("/home/alan/Documents/SatDump_ReWork/build/noaa_mhs_test_gac/AVHRR/product.cbor", "NOAA-19 GAC");
        // loadProductsInViewer("/home/alan/Documents/SatDump_ReWork/build/npp_hrd_new/VIIRS/product.cbor", "JPSS-1 HRD");
        // loadProductsInViewer("/home/alan/Documents/SatDump_ReWork/build/m2_lrpt_test/MSU-MR/product.cbor", "METEOR-M2 LRPT");
    }

    void ViewerApplication::loadDatasetInViewer(std::string path)
    {
        ProductDataSet dataset;
        dataset.load(path);

        std::string dataset_name = dataset.satellite_name + " " + timestamp_to_string(dataset.timestamp);

        int i = -1;
        bool contains = false;
        do
        {
            contains = false;
            std::string curr_name = ((i + 1) == 0 ? dataset_name : (dataset_name + " #" + std::to_string(i + 1)));
            for (int i = 0; i < (int)products_and_handlers.size(); i++)
                if (products_and_handlers[i]->dataset_name == curr_name)
                    contains = true;
            i++;
        } while (contains);
        dataset_name = (i == 0 ? dataset_name : (dataset_name + " #" + std::to_string(i)));

        std::string pro_directory = std::filesystem::path(path).parent_path().string();
        for (std::string pro_path : dataset.products_list)
            loadProductsInViewer(pro_directory + "/" + pro_path, dataset_name);
    }

    void ViewerApplication::loadProductsInViewer(std::string path, std::string dataset_name)
    {
        if (std::filesystem::exists(path))
        {
            std::shared_ptr<Products> products = loadProducts(path);

            // Get instrument settings
            nlohmann::ordered_json instrument_viewer_settings;
            if (config::main_cfg["viewer"]["instruments"].contains(products->instrument_name))
                instrument_viewer_settings = config::main_cfg["viewer"]["instruments"][products->instrument_name];
            else
                logger->error("Unknown instrument : {:s}!", products->instrument_name);

            // Init Handler
            std::string handler_id;
            if (instrument_viewer_settings.contains("handler"))
                handler_id = instrument_viewer_settings["handler"].get<std::string>();
            else if (products->contents["type"] == "image")
                handler_id = "image_handler";
            else if (products->contents["type"] == "radiation")
                handler_id = "radiation_handler";
            else if (products->contents["type"] == "scatterometer")
                handler_id = "scatterometer_handler";
            logger->debug("Using handler {:s} for instrument {:s}", handler_id, products->instrument_name);
            std::shared_ptr<ViewerHandler> handler = viewer_handlers_registry[handler_id]();

            handler->products = products.get();
            handler->instrument_cfg = instrument_viewer_settings;
            handler->init();

            if (dataset_name != "")
            {
                bool dataset_exists = false;
                for (std::string dataset_name2 : opened_datasets)
                    dataset_exists = dataset_exists | (dataset_name2 == dataset_name);

                if (!dataset_exists)
                    opened_datasets.push_back(dataset_name);
            }

            // Push products and handler
            products_and_handlers.push_back(std::make_shared<ProductsHandler>(products, handler, dataset_name));
        }
    }

    ViewerApplication::~ViewerApplication()
    {
        config::main_cfg["user"]["viewer_state"]["panel_ratio"] = panel_ratio;
    }

    ImRect ViewerApplication::renderHandler(ProductsHandler &ph, int index)
    {
        std::string label = ph.products->instrument_name;
        if (ph.handler->instrument_cfg.contains("name"))
            label = ph.handler->instrument_cfg["name"].get<std::string>();

        ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | (index == current_handler_id ? ImGuiTreeNodeFlags_Selected : 0));
        if (ImGui::IsItemClicked())
            current_handler_id = index;

        if (index == current_handler_id && ph.dataset_name == "")
        { // Closing button
            ImGui::SameLine();
            ImGui::Text("  ");
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 0, 0, 255));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            if (ImGui::SmallButton(std::string(u8"\uf00d##" + ph.dataset_name + label).c_str()))
            {
                logger->warn("Closing products " + label);
                ph.marked_for_close = true;
            }
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();
        }

        ImRect rect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

        if (index == current_handler_id)
        {
            ImGui::TreePush();
            products_and_handlers[current_handler_id]->handler->drawTreeMenu();
            ImGui::TreePop();
        }
        // ImGui::InputInt("Current Product", &current_handler_id);

        return rect;
    }

    void ViewerApplication::drawPanel()
    {
        if (ImGui::BeginTabBar("Viewer Prob Tabbar", ImGuiTabBarFlags_None))
        {
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 2);
            if (ImGui::BeginTabItem("Products###productsviewertab"))
            {
                if (current_selected_tab != 0)
                    current_selected_tab = 0;

                if (ImGui::CollapsingHeader("General", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    for (std::string dataset_name : opened_datasets)
                    {
                        if (products_cnt_in_dataset(dataset_name))
                        {
                            ImGui::TreeNodeEx(dataset_name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                            ImGui::TreePush();
                            for (int i = 0; i < (int)products_and_handlers.size(); i++)
                                if (products_and_handlers[i]->dataset_name == dataset_name && products_and_handlers[i]->handler->shouldProject())
                                {
                                    SelectableColor(IM_COL32(186, 153, 38, 65));
                                    break;
                                }

                            { // Closing button
                                ImGui::SameLine();
                                ImGui::Text("  ");
                                ImGui::SameLine();

                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 0, 0, 255));
                                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                                if (ImGui::SmallButton(std::string(u8"\uf00d##dataset" + dataset_name).c_str()))
                                {
                                    logger->warn("Closing datset " + dataset_name);
                                    for (int i = 0; i < (int)products_and_handlers.size(); i++)
                                        if (products_and_handlers[i]->dataset_name == dataset_name)
                                            products_and_handlers[i]->marked_for_close = true;
                                }
                                ImGui::PopStyleColor();
                                ImGui::PopStyleColor();
                            }

                            const ImColor TreeLineColor = ImColor(128, 128, 128, 255); // ImGui::GetColorU32(ImGuiCol_Text);
                            const float SmallOffsetX = 11.0f;                          // for now, a hardcoded value; should take into account tree indent size
                            ImDrawList *drawList = ImGui::GetWindowDrawList();

                            ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();
                            verticalLineStart.x += SmallOffsetX; // to nicely line up with the arrow symbol
                            ImVec2 verticalLineEnd = verticalLineStart;

                            for (int i = 0; i < (int)products_and_handlers.size(); i++)
                            {
                                if (products_and_handlers[i]->dataset_name == dataset_name)
                                {
                                    const float HorizontalTreeLineSize = 8.0f * ui_scale;                 // chosen arbitrarily
                                    const ImRect childRect = renderHandler(*products_and_handlers[i], i); // RenderTree(child);
                                    const float midpoint = (childRect.Min.y + childRect.Max.y) / 2.0f;
                                    drawList->AddLine(ImVec2(verticalLineStart.x, midpoint), ImVec2(verticalLineStart.x + HorizontalTreeLineSize, midpoint), TreeLineColor);
                                    verticalLineEnd.y = midpoint;
                                    if (products_and_handlers[i]->handler->shouldProject())
                                        SelectableColor(IM_COL32(186, 153, 38, 65));
                                }
                            }

                            drawList->AddLine(verticalLineStart, verticalLineEnd, TreeLineColor);

                            ImGui::TreePop();
                        }
                    }

                    // Render unclassified
                    if (products_cnt_in_dataset(""))
                    {
                        ImGui::TreeNodeEx("Others", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                        ImGui::TreePush();
                        for (int i = 0; i < (int)products_and_handlers.size(); i++)
                            if (products_and_handlers[i]->dataset_name == "")
                                renderHandler(*products_and_handlers[i], i);
                        ImGui::TreePop();
                    }

                    // Handle deletion if required
                    for (int i = 0; i < (int)products_and_handlers.size(); i++)
                    {
                        if (products_and_handlers[i]->marked_for_close)
                        {
                            products_and_handlers.erase(products_and_handlers.begin() + i);
                            if (current_handler_id >= (int)products_and_handlers.size())
                                current_handler_id = 0;
                            break;
                        }
                    }

                    ImGui::Separator();
                    ImGui::Text("Load Dataset :");
                    if (select_dataset_dialog.draw())
                    {
                        try
                        {
                            loadDatasetInViewer(select_dataset_dialog.getPath());
                        }
                        catch (std::exception &e)
                        {
                            error::set_error("Error opening dataset!", e.what());
                        }
                    }
                    ImGui::Text("Load Products :");
                    if (select_products_dialog.draw())
                    {
                        try
                        {
                            loadProductsInViewer(select_products_dialog.getPath());
                        }
                        catch (std::exception &e)
                        {
                            error::set_error("Error opening products!", e.what());
                        }
                    }
                }

                if (products_and_handlers.size() > 0)
                    products_and_handlers[current_handler_id]->handler->drawMenu();

                ImGui::EndTabItem();
            }

            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 2);
            if (ImGui::BeginTabItem("Projections###projssviewertab"))
            {
                if (current_selected_tab != 1)
                {
                    current_selected_tab = 1;
                    refreshProjectionLayers();
                }
                drawProjectionPanel();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    void ViewerApplication::drawContent()
    {
    }

    void ViewerApplication::drawUI()
    {
        /*
        ImVec2 viewer_size = ImGui::GetContentRegionAvail();
        ImGui::BeginGroup();
        ImGui::BeginChild("ViewerChildPanel", {float(viewer_size.x * panel_ratio), float(viewer_size.y)}, false);
        {
            drawPanel();
        }
        ImGui::EndChild();
        ImGui::EndGroup();

        ImVec2 mouse_pos = ImGui::GetMousePos();
        if ((mouse_pos.x > viewer_size.x * panel_ratio + 15 * ui_scale - 10 * ui_scale &&
             mouse_pos.x < viewer_size.x * panel_ratio + 15 * ui_scale + 10 * ui_scale &&
             mouse_pos.y > 35 * ui_scale) ||
            dragging_panel)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                float new_x = mouse_pos.x;
                float new_ratio = new_x / viewer_size.x;
                if (new_ratio > 0.1 && new_ratio < 0.9)
                    panel_ratio = new_ratio;
            }
            else
                dragging_panel = false;
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                dragging_panel = true;
        }
        else if (ImGui::GetMouseCursor() != ImGuiMouseCursor_ResizeNS)
            ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

        ImGui::SameLine();

        ImGui::BeginGroup();
        if (current_selected_tab == 0)
        {
            if (products_and_handlers.size() > 0)
                products_and_handlers[current_handler_id]->handler->drawContents({float(viewer_size.x * (1.0 - panel_ratio) - 4), float(viewer_size.y)});
            else
                ImGui::GetWindowDrawList()
                    ->AddRectFilled(ImGui::GetCursorScreenPos(),
                                    ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x,
                                           ImGui::GetCursorScreenPos().y + ImGui::GetContentRegionAvail().y),
                                    ImColor::HSV(0, 0, 0));
        }
        else if (current_selected_tab == 1)
        {
            projection_image_widget.draw({float(viewer_size.x * (1.0 - panel_ratio) - 4), float(viewer_size.y)});
        }
        ImGui::EndGroup();
        */

        ImVec2 viewer_size = ImGui::GetContentRegionAvail();

        if (ImGui::BeginTable("##wiever_table", 2, ImGuiTableFlags_NoBordersInBodyUntilResize | ImGuiTableFlags_NoPadInnerX | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("##panel_v", ImGuiTableColumnFlags_WidthFixed, viewer_size.x * panel_ratio);
            ImGui::TableSetupColumn("##view", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableNextColumn();
            ImGui::BeginChild("ViewerChildPanel", {float(viewer_size.x * panel_ratio), float(viewer_size.y - 10)}, false);
            {
                drawPanel();
            }
            ImGui::EndChild();
            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) || last_width != viewer_size.x)
                panel_ratio = ImGui::GetColumnWidth() / viewer_size[0];
            last_width = viewer_size.x;
            ImGui::TableNextColumn();
            ImGui::BeginGroup();
            if (current_selected_tab == 0)
            {
                if (products_and_handlers.size() > 0)
                    products_and_handlers[current_handler_id]->handler->drawContents({float(viewer_size.x * (1.0 - panel_ratio) - 4), float(viewer_size.y)});
                else
                    ImGui::GetWindowDrawList()
                        ->AddRectFilled(ImGui::GetCursorScreenPos(),
                                        ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x,
                                               ImGui::GetCursorScreenPos().y + ImGui::GetContentRegionAvail().y),
                                        ImColor::HSV(0, 0, 0));
            }
            else if (current_selected_tab == 1)
            {
                projection_image_widget.draw({float(viewer_size.x * (1.0 - panel_ratio) - 4), float(viewer_size.y)});
            }
            ImGui::EndGroup();
            ImGui::EndTable();
        }
    }

    std::map<std::string, std::function<std::shared_ptr<ViewerHandler>()>> viewer_handlers_registry;
    void registerViewerHandlers()
    {
        viewer_handlers_registry.emplace(ImageViewerHandler::getID(), ImageViewerHandler::getInstance);
        viewer_handlers_registry.emplace(RadiationViewerHandler::getID(), RadiationViewerHandler::getInstance);
        viewer_handlers_registry.emplace(ScatterometerViewerHandler::getID(), ScatterometerViewerHandler::getInstance);
    }
};