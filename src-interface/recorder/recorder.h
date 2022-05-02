#pragma once

#include "../app.h"
#include "products/products.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace satdump
{
    class RecorderApplication : public Application
    {
    protected:
        virtual void drawUI();

    public:
        RecorderApplication();
        ~RecorderApplication();

    public:
        static std::string getID() { return "recorder"; }
        static std::shared_ptr<Application> getInstance() { return std::make_shared<RecorderApplication>(); }
    };
};