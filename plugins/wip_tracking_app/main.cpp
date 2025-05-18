#include "core/module.h"
#include "core/plugin.h"
#include "logger.h"

#include "app.h"
#include "tracking.h"
#include "viewer2/viewer.h"

class WipTrackingAppPlugin : public satdump::Plugin
{
public:
    std::string getID() { return "wiptracking_app"; }

    void init()
    {
        // TODOREWORK maybe a way to call up/init a handler?
        satdump::eventBus->register_handler<satdump::viewer::ViewerApplication::RenderLoadMenuElementsEvent>(renderViewerLoaderButton);
    }

    static void renderViewerLoaderButton(const satdump::viewer::ViewerApplication::RenderLoadMenuElementsEvent &evt)
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::BeginMenu("Others"))
            {
                if (ImGui::MenuItem("WIP Tracking"))
                    evt.master_handler->addSubHandler(std::make_shared<satdump::WipTrackingHandler>());
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
    } // TODOREWORK
};

PLUGIN_LOADER(WipTrackingAppPlugin)