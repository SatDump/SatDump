#include "viewer.h"
#include "imgui/imgui.h"

namespace satdump
{
    ViewerApplication::ViewerApplication()
        : Application("viewer")
    {
        handler = new ImageViewerHandler();

        //pro.load("/home/alan/Documents/SatDump_ReWork/build/metop_ahrpt_new/AVHRR/product.cbor");
        pro.load("/home/zbyszek/Downloads/test/AVHRR/product.cbor");
        handler->products = &pro;
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
};