#include "viewer.h"
#include "imgui/imgui.h"

namespace satdump
{
    ViewerApplication::ViewerApplication()
        : Application("viewer")
    {
        handler = new ImageViewerHandler();
        image::Image<uint16_t> image;
        // image.load_png("/home/alan/Downloads/probaaa.png");
        ((ImageViewerHandler *)handler)->image_view.update(image);
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
        handler->drawContents({viewer_size.x * 0.80, viewer_size.y});
        ImGui::EndGroup();
    }
};