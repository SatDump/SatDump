#include "viewer.h"
#include "image_handler.h"
#include "core/config.h"

namespace satdump
{
    ViewerApplication::ViewerApplication()
        : Application("viewer")
    {
        // pro.load("/home/alan/Documents/SatDump_ReWork/build/metop_ahrpt_new/AVHRR/product.cbor");
        //  pro.load("/home/alan/Documents/SatDump_ReWork/build/aqua_test_new/MODIS/product.cbor");

        loadProductsInViewer("/home/alan/Documents/SatDump_ReWork/build/noaa_mhs_test/AMSU/product.cbor");
        loadProductsInViewer("/home/alan/Documents/SatDump_ReWork/build/metop_ahrpt_new/AVHRR/product.cbor");
        loadProductsInViewer("/home/alan/Documents/SatDump_ReWork/build/noaa_mhs_test/MHS/product.cbor");
        loadProductsInViewer("/home/alan/Documents/SatDump_ReWork/build/noaa_mhs_test_gac/MHS/product.cbor");
        loadProductsInViewer("/home/alan/Documents/SatDump_ReWork/build/noaa_mhs_test_gac/HIRS/product.cbor");
        loadProductsInViewer("/home/alan/Documents/SatDump_ReWork/build/noaa_mhs_test_gac/AVHRR/product.cbor");
    }

    void ViewerApplication::loadProductsInViewer(std::string path)
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
            std::string handler_id = instrument_viewer_settings["handler"].get<std::string>();
            logger->debug("Using handler {:s} for instrument {:s}", handler_id, products->instrument_name);
            std::shared_ptr<ViewerHandler> handler = viewer_handlers_registry[handler_id]();

            handler->products = products.get();
            handler->instrument_cfg = instrument_viewer_settings;
            handler->init();

            // Push products and handler
            products_and_handlers.push_back({products, handler});
        }
    }

    ViewerApplication::~ViewerApplication()
    {
    }

    void ViewerApplication::drawPanel()
    {
        if (ImGui::CollapsingHeader("General"))
        {
            for (int i = 0; i < products_and_handlers.size(); i++)
            {
                std::string label = products_and_handlers[i].first->instrument_name;
                if (products_and_handlers[i].second->instrument_cfg.contains("name"))
                    label = products_and_handlers[i].second->instrument_cfg["name"].get<std::string>();

                ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet | (i == current_handler_id ? ImGuiTreeNodeFlags_Selected : 0));
                if (ImGui::IsItemClicked())
                    current_handler_id = i;

                if (i == current_handler_id)
                {
                    ImGui::TreePush();
                    products_and_handlers[current_handler_id].second->drawTreeMenu();
                    ImGui::TreePop();
                }
                // ImGui::InputInt("Current Product", &current_handler_id);
            }
        }
    }

    void ViewerApplication::drawContent()
    {
    }

    void ViewerApplication::drawUI()
    {
        ImVec2 viewer_size = ImGui::GetContentRegionAvail();
        ImGui::BeginGroup();
        ImGui::BeginChild("ViewerChildPanel", {viewer_size.x * 0.20, viewer_size.y}, false);
        {
            drawPanel();
            products_and_handlers[current_handler_id].second->drawMenu();
        }
        ImGui::EndChild();
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        products_and_handlers[current_handler_id].second->drawContents({viewer_size.x * 0.80 - 4, viewer_size.y});
        ImGui::EndGroup();
    }

    std::map<std::string, std::function<std::shared_ptr<ViewerHandler>()>> viewer_handlers_registry;
    void registerViewerHandlers()
    {
        viewer_handlers_registry.emplace(ImageViewerHandler::getID(), ImageViewerHandler::getInstance);
    }
};