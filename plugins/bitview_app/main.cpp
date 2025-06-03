#include "core/plugin.h"
#include "logger.h"

#include "app.h"
#include "bitview.h"
#include "viewer2/viewer.h"

class BitViewAppPlugin : public satdump::Plugin
{
public:
    std::string getID() { return "bitview_app"; }

    void init()
    {
        // TODOREWORK maybe a way to call up/init a handler?
        satdump::eventBus->register_handler<satdump::viewer::ViewerApplication::RenderLoadMenuElementsEvent>(renderViewerLoaderButton);
    }

    static void renderViewerLoaderButton(const satdump::viewer::ViewerApplication::RenderLoadMenuElementsEvent &evt)
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::BeginMenu("Add"))
            {
                if (ImGui::MenuItem("BitView"))
                    evt.master_handler->addSubHandler(std::make_shared<satdump::BitViewHandler>());
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
    } // TODOREWORK
};

PLUGIN_LOADER(BitViewAppPlugin)