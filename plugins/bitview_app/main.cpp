#include "bit_container.h"
#include "cli/cmd.h"
#include "common/widgets/menuitem_fileopen.h"
#include "core/cli/cli.h"
#include "core/plugin.h"
#include "explorer/explorer.h"
#include "logger.h"

#include "bitview.h"
#include "explorer/explorer.h"
#include <filesystem>
#include <memory>

class BitViewAppPlugin : public satdump::Plugin
{
public:
    std::string getID() { return "bitview_app"; }

    void init()
    {
        // TODOREWORK maybe a way to call up/init a handler?
        satdump::eventBus->register_handler<satdump::explorer::RenderLoadMenuElementsEvent>(renderExplorerLoaderButton);

        satdump::eventBus->register_handler<satdump::cli::RegisterSubcommandEvent>(registerCliCommands);
    }

    static void registerCliCommands(const satdump::cli::RegisterSubcommandEvent &evt) { evt.cmd_handlers.push_back(std::make_shared<satdump::BitViewCmdHandler>()); }

    static satdump::widget::MenuItemFileOpen fopen_menu;

    static void renderExplorerLoaderButton(const satdump::explorer::RenderLoadMenuElementsEvent &evt)
    {
        if (ImGui::BeginMenu("Add"))
        {
            if (ImGui::BeginMenu("Tools"))
            {
                fopen_menu.render("BitView", "Open binary file", "", {{"All Files", "*"}});
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        if (fopen_menu.update())
            evt.master_handler->addSubHandler(std::make_shared<satdump::BitViewHandler>(std::make_shared<satdump::BitContainer>(                //
                std::filesystem::path(fopen_menu.getPath()).stem().string() + std::filesystem::path(fopen_menu.getPath()).extension().string(), //
                fopen_menu.getPath())));
    }
};

satdump::widget::MenuItemFileOpen BitViewAppPlugin::fopen_menu;

PLUGIN_LOADER(BitViewAppPlugin)