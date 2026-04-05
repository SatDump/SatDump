#include "core/plugin.h"
#include "explorer/explorer.h"

#include "explorer/explorer.h"
#include "satdump_downconverter/satdump_downconverter_handler.h"

class ExperimentalDevicesSupportPlugin : public satdump::Plugin
{
public:
    std::string getID() { return "experimental_devices_support"; }

    void init()
    {
        // TODOREWORK maybe a way to call up/init a handler?
        satdump::eventBus->register_handler<satdump::explorer::RenderLoadMenuElementsEvent>(renderExplorerLoaderButton);
    }

    static void renderExplorerLoaderButton(const satdump::explorer::RenderLoadMenuElementsEvent &evt)
    {
        if (ImGui::BeginMenu("Add"))
        {
            if (ImGui::BeginMenu("Experimental"))
            {
                if (ImGui::MenuItem("SatDump Downconverter UI"))
                    evt.master_handler->addSubHandler(std::make_shared<satdump::exp_devs::SatDumpDownconverHandler>());
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
    }
};

PLUGIN_LOADER(ExperimentalDevicesSupportPlugin)