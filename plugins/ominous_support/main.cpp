#include "core/plugin.h"
#include "explorer/explorer.h"
#include "logger.h"

#include "explorer/explorer.h"
#include "ominous/module_ominous_instruments.h"
#include "pipeline/module.h"
#include "simulator/simulator_handler.h"

class OminousSupportPlugin : public satdump::Plugin
{
public:
    std::string getID() { return "ominous_support"; }

    void init()
    {
        // TODOREWORK maybe a way to call up/init a handler?
        satdump::eventBus->register_handler<satdump::explorer::RenderLoadMenuElementsEvent>(renderExplorerLoaderButton);
        satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler);
    }

    static void renderExplorerLoaderButton(const satdump::explorer::RenderLoadMenuElementsEvent &evt)
    {
        if (ImGui::BeginMenu("Add"))
        {
            if (ImGui::BeginMenu("Tools"))
            {
                if (ImGui::MenuItem("Ominous Simulator"))
                    evt.master_handler->addSubHandler(std::make_shared<satdump::ominous::SimulatorHandler>());
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
    }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, ominous::instruments::OminousInstrumentsDecoderModule); }
};

PLUGIN_LOADER(OminousSupportPlugin)