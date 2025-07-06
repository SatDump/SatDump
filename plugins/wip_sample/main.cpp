#include "core/plugin.h"
#include "logger.h"

#include "app.h"
#include "explorer/explorer.h"
#include "wip_sample.h"

class WipSamplePlugin : public satdump::Plugin
{
public:
    std::string getID() { return "wip_sample_app"; }

    void init()
    {
        // TODOREWORK maybe a way to call up/init a handler?
        satdump::eventBus->register_handler<satdump::explorer::ExplorerApplication::RenderLoadMenuElementsEvent>(renderExplorerLoaderButton);
    }

    static void renderExplorerLoaderButton(const satdump::explorer::ExplorerApplication::RenderLoadMenuElementsEvent &evt)
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::BeginMenu("Tools"))
            {
                if (ImGui::MenuItem("WIP Sample"))
                    evt.master_handler->addSubHandler(std::make_shared<satdump::WipSampleHandler>());
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
    } // TODOREWORK
};

PLUGIN_LOADER(WipSamplePlugin)
