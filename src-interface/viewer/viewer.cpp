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

        products = loadProducts("/home/alan/Documents/SatDump_ReWork/build/noaa_mhs_test/MHS/product.cbor");
        // products = loadProducts("/home/alan/Documents/SatDump_ReWork/build/metop_ahrpt_new/AVHRR/product.cbor");

        // Get instrument settings
        nlohmann::ordered_json instrument_viewer_settings;
        if (config::main_cfg["viewer"]["instruments"].contains(products->instrument_name))
            instrument_viewer_settings = config::main_cfg["viewer"]["instruments"][products->instrument_name];
        else
            logger->error("Unknown instrument : {:s}!", products->instrument_name);

        std::string handler_id = instrument_viewer_settings["handler"].get<std::string>();
        logger->debug("Using handler {:s} for instrument {:s}", handler_id, products->instrument_name);
        handler = viewer_handlers_registry[handler_id]();

        handler->products = products.get();
        handler->instrument_cfg = instrument_viewer_settings;
        handler->init();
        // image::Image<uint16_t> image;
        // image.load_png("/home/alan/Downloads/probaaa.png");
        // ((ImageViewerHandler *)handler)->image_view.update(image);
    }

    ViewerApplication::~ViewerApplication()
    {
    }

    void ViewerApplication::drawPanel()
    {
        if (ImGui::CollapsingHeader("General"))
        {
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
            handler->drawMenu();
        }
        ImGui::EndChild();
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        handler->drawContents({viewer_size.x * 0.80 - 4, viewer_size.y});
        ImGui::EndGroup();
    }

    std::map<std::string, std::function<std::shared_ptr<ViewerHandler>()>> viewer_handlers_registry;
    void registerViewerHandlers()
    {
        viewer_handlers_registry.emplace(ImageViewerHandler::getID(), ImageViewerHandler::getInstance);
    }
};