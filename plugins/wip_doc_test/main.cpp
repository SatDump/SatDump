#include "core/plugin.h"
#include "logger.h"

#include "doctest.h"
#include "explorer/explorer.h"

class DocTestAppPlugin : public satdump::Plugin
{
public:
    std::string getID() { return "doctest_app"; }

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
                if (ImGui::MenuItem("WIP Documentation"))
                    evt.master_handler->addSubHandler(std::make_shared<satdump::DocTestHandler>());
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
    } // TODOREWORK
};

PLUGIN_LOADER(DocTestAppPlugin)