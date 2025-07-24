#include "core/plugin.h"
#include "explorer/explorer.h"
#include "logger.h"

#include "bitview.h"
#include "explorer/explorer.h"

class BitViewAppPlugin : public satdump::Plugin
{
public:
    std::string getID() { return "bitview_app"; }

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
                if (ImGui::MenuItem("BitView"))
                    evt.master_handler->addSubHandler(std::make_shared<satdump::BitViewHandler>());
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
    }
};

PLUGIN_LOADER(BitViewAppPlugin)