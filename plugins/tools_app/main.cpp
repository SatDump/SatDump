#include "cli/hard2soft.h"
#include "core/cli/cli.h"
#include "core/plugin.h"
#include "explorer/explorer.h"
#include "lutgen/lut_generator.h"

class ToolsAppPlugin : public satdump::Plugin
{
public:
    std::string getID() { return "tools_app"; }

    void init()
    {
        // TODOREWORK maybe a way to call up/init a handler?
        satdump::eventBus->register_handler<satdump::explorer::RenderLoadMenuElementsEvent>(renderExplorerLoaderButton);
        satdump::eventBus->register_handler<satdump::cli::RegisterSubcommandEvent>(registerCliCommands);
    }

    static void registerCliCommands(const satdump::cli::RegisterSubcommandEvent &evt) { evt.cmd_handlers.push_back(std::make_shared<satdump::HardToSoftCmdHandler>()); }

    static void renderExplorerLoaderButton(const satdump::explorer::RenderLoadMenuElementsEvent &evt)
    {
        if (ImGui::BeginMenu("Add"))
        {
            if (ImGui::BeginMenu("Tools"))
            {
                if (ImGui::MenuItem("LUT Generator"))
                    evt.master_handler->addSubHandler(std::make_shared<satdump::lutgen::LutGeneratorHandler>());
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
    } // TODOREWORK
};

PLUGIN_LOADER(ToolsAppPlugin)