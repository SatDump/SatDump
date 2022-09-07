#include "viewer.h"
#include "image_handler.h"
#include "radiation_handler.h"
#include "core/config.h"
#include "products/dataset.h"
#include "common/utils.h"
#include "error.h"

namespace satdump
{
    ViewerApplication::ViewerApplication()
        : Application("viewer")
    {
        // pro.load("/home/alan/Documents/SatDump_ReWork/build/metop_ahrpt_new/AVHRR/product.cbor");
        //  pro.load("/home/alan/Documents/SatDump_ReWork/build/aqua_test_new/MODIS/product.cbor");

        // loadDatasetInViewer("/home/alan/Documents/SatDump_ReWork/build/metop_ahrpt_new/dataset.json");

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

        std::string pro_directory = std::filesystem::path(path).parent_path().string();
        for (std::string pro_path : dataset.products_list)
            loadProductsInViewer(pro_directory + "/" + pro_path, dataset.satellite_name + " " + timestamp_to_string(dataset.timestamp));
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
            products_and_handlers.push_back({products, handler, dataset_name});
        }
    }

    ViewerApplication::~ViewerApplication()
    {
    }

    ImRect ViewerApplication::renderHandler(ProductsHandler &ph, int index)
    {
        std::string label = ph.products->instrument_name;
        if (ph.handler->instrument_cfg.contains("name"))
            label = ph.handler->instrument_cfg["name"].get<std::string>();

        ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | (index == current_handler_id ? ImGuiTreeNodeFlags_Selected : 0));
        if (ImGui::IsItemClicked())
            current_handler_id = index;

        ImRect rect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

        if (index == current_handler_id)
        {
            ImGui::TreePush();
            products_and_handlers[current_handler_id].handler->drawTreeMenu();
            ImGui::TreePop();
        }
        // ImGui::InputInt("Current Product", &current_handler_id);

        return rect;
    }

    void ViewerApplication::drawPanel()
    {
        if (ImGui::BeginTabBar("Viewer Prob Tabbar", ImGuiTabBarFlags_None))
        {
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth()/2);
            if (ImGui::BeginTabItem("Products"))
            {
                if (ImGui::CollapsingHeader("General"), ImGuiTreeNodeFlags_DefaultOpen)
                {
                    for (std::string dataset_name : opened_datasets)
                    {
                        if (products_cnt_in_dataset(dataset_name))
                        {
                            ImGui::TreeNodeEx(dataset_name.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
                            ImGui::TreePush();

                            const ImColor TreeLineColor = ImColor(128, 128, 128, 255); // ImGui::GetColorU32(ImGuiCol_Text);
                            const float SmallOffsetX = 11.0f;                          // for now, a hardcoded value; should take into account tree indent size
                            ImDrawList *drawList = ImGui::GetWindowDrawList();

                            ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();
                            verticalLineStart.x += SmallOffsetX; // to nicely line up with the arrow symbol
                            ImVec2 verticalLineEnd = verticalLineStart;

                            for (int i = 0; i < (int)products_and_handlers.size(); i++)
                                if (products_and_handlers[i].dataset_name == dataset_name)
                                {
                                    const float HorizontalTreeLineSize = 8.0f;                           // chosen arbitrarily
                                    const ImRect childRect = renderHandler(products_and_handlers[i], i); // RenderTree(child);
                                    const float midpoint = (childRect.Min.y + childRect.Max.y) / 2.0f;
                                    drawList->AddLine(ImVec2(verticalLineStart.x, midpoint), ImVec2(verticalLineStart.x + HorizontalTreeLineSize, midpoint), TreeLineColor);
                                    verticalLineEnd.y = midpoint;
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
                            if (products_and_handlers[i].dataset_name == "")
                                renderHandler(products_and_handlers[i], i);
                        ImGui::TreePop();
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
                            error::set_error("Error opening dataset!", e.what());
                        }
                    }
                }
                ImGui::EndTabItem();
            }
            ImGui::SetNextItemWidth(ImGui::GetWindowWidth()/2);
            if (ImGui::BeginTabItem("Projections"))
            {
                if (ImGui::CollapsingHeader("Projection"))
                {
                    int itm = 0;
                    float tl_lon = -180;
                    float tl_lat = 90;
                    float br_lon = 180;
                    float br_lat = -90;
                    ImGui::Combo("##targetproj", &itm, "Equirectangular");
                    ImGui::Text("Top Left :");
                    ImGui::InputFloat("Lon##tl", &tl_lon);
                    // ImGui::SameLine();
                    ImGui::InputFloat("Lat##tl", &tl_lat);
                    ImGui::Text("Bottom Right :");
                    ImGui::InputFloat("Lon##br", &br_lon);
                    // ImGui::SameLine();
                    ImGui::InputFloat("Lat##br", &br_lat);
                }
                if (ImGui::CollapsingHeader("Layers"))
                {
                    int itm_radio = 0;
                    ImGui::Text("Mode :");
                    ImGui::RadioButton("Blend", &itm_radio, 0);
                    ImGui::SameLine();
                    ImGui::RadioButton("Overlay", &itm_radio, 1);

                    ImGui::Text("Layers :");
                    float opacity = 100;

                    // ImGui::Text("MetOp AVHRR idk");
                    // ImGui::InputFloat("Opacity", &opacity);

                    if (ImGui::BeginListBox("##pipelineslistbox"))
                    {
                        // ImGui::Selectable
                        ImGui::EndListBox();
                    }
                }
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
        ImVec2 viewer_size = ImGui::GetContentRegionAvail();
        ImGui::BeginGroup();
        ImGui::BeginChild("ViewerChildPanel", {float(viewer_size.x * 0.20), float(viewer_size.y)}, false);
        {
            drawPanel();
            if (products_and_handlers.size() > 0)
                products_and_handlers[current_handler_id].handler->drawMenu();
        }
        ImGui::EndChild();
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        if (products_and_handlers.size() > 0)
            products_and_handlers[current_handler_id].handler->drawContents({float(viewer_size.x * 0.80 - 4), float(viewer_size.y)});
        else
            ImGui::GetWindowDrawList()
                ->AddRectFilled(ImGui::GetCursorScreenPos(),
                                ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x,
                                       ImGui::GetCursorScreenPos().y + ImGui::GetContentRegionAvail().y),
                                ImColor::HSV(0, 0, 0));
        ImGui::EndGroup();
    }

    std::map<std::string, std::function<std::shared_ptr<ViewerHandler>()>> viewer_handlers_registry;
    void registerViewerHandlers()
    {
        viewer_handlers_registry.emplace(ImageViewerHandler::getID(), ImageViewerHandler::getInstance);
        viewer_handlers_registry.emplace(RadiationViewerHandler::getID(), RadiationViewerHandler::getInstance);
    }
};