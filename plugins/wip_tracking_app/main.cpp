#include "core/plugin.h"
#include "logger.h"

#include "explorer/explorer.h"
#include "tracking.h"

class WipTrackingAppPlugin : public satdump::Plugin
{
public:
    std::string getID() { return "wiptracking_app"; }

    void init()
    {
        // TODOREWORK maybe a way to call up/init a handler?
        satdump::eventBus->register_handler<satdump::explorer::RenderLoadMenuElementsEvent>(renderExplorerLoaderButton);
    }

    static void renderExplorerLoaderButton(const satdump::explorer::RenderLoadMenuElementsEvent &evt)
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::BeginMenu("Tools"))
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