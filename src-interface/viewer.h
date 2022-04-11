#pragma once

#include "app.h"

#include "products/products.h"
#include "common/widgets/image_view.h"
#include "imgui/imgui_stdlib.h"

namespace satdump
{
    class ViewerHandler
    {
    public:
        Products products;

        virtual void init() = 0;
        virtual void drawMenu() = 0;
        virtual void drawContents(ImVec2 win_size) = 0;
    };

    class ImageViewerHandler : public ViewerHandler
    {
    public:
        // Image objects
        image::Image<uint16_t> image_obj;
        ImageViewWidget image_view;

        // RGB Handling
        std::string rgb_equation;
        float rgb_progress = 0;

        void init()
        {
        }

        void drawMenu()
        {
            if (ImGui::CollapsingHeader("RGB Composites"))
            {
                ImGui::InputText("Equation", &rgb_equation);
            }
        }

        void drawContents(ImVec2 win_size)
        {
            image_view.draw(win_size);
        }
    };

    class ViewerApplication : public Application
    {
    protected:
        const std::string app_id;
        virtual void drawUI();

        virtual void drawPanel();
        virtual void drawContent();

    public:
        ViewerApplication();
        ~ViewerApplication();

        ViewerHandler *handler;

    public:
        static std::string getID() { return "viewer"; }
        static std::shared_ptr<Application> getInstance() { return std::make_shared<ViewerApplication>(); }
    };
};