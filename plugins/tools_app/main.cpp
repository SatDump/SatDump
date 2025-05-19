#include "core/plugin.h"
#include "lutgen/lut_generator.h"
#include "viewer2/viewer.h"

class ToolsAppPlugin : public satdump::Plugin
{
public:
    std::string getID() { return "tools_app"; }

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
                if (ImGui::MenuItem("Lut Generator"))
                    evt.master_handler->addSubHandler(std::make_shared<satdump::lutgen::LutGeneratorHandler>());
                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }
    } // TODOREWORK
};

PLUGIN_LOADER(ToolsAppPlugin)