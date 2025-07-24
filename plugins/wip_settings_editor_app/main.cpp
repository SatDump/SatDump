#include "core/plugin.h"
#include "logger.h"

#include "explorer/explorer.h"
#include "settings_editor.h"

class WipSettingsEditorAppPlugin : public satdump::Plugin
{
public:
    std::string getID() { return "wipsettings_app"; }

    void init() { satdump::eventBus->register_handler<satdump::explorer::RenderLoadMenuElementsEvent>(renderExplorerLoaderButton); }

    static void renderExplorerLoaderButton(const satdump::explorer::RenderLoadMenuElementsEvent &evt)
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::BeginMenu("Tools"))
            {
                if (ImGui::MenuItem("Instrument Config Editor"))
                    evt.master_handler->addSubHandler(std::make_shared<satdump::WipSettingsEditorHandler>());
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
    }
};

PLUGIN_LOADER(WipSettingsEditorAppPlugin)