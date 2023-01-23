#define SATDUMP_DLL_EXPORT2 1
#include "core/plugin.h"
#include "app.h"
#include "imgui/imgui.h"
#include "viewer/viewer.h"
#include "recorder/recorder.h"

namespace satdump
{
    Application::Application(std::string id)
        : app_id(id)
    {
    }

    Application::~Application()
    {
    }

    void Application::drawWindow()
    {
        ImGui::Begin(app_id.c_str());
        drawUI();
        ImGui::End();
    }

    void Application::draw()
    {
        ImGui::BeginChild(app_id.c_str(), ImGui::GetContentRegionAvail(), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        drawUI();
        ImGui::EndChild();
    }

    void Application::drawUI()
    {
        ImGui::Text("Nothing implemented there yet!");
    }

    SATDUMP_DLL2 std::map<std::string, std::function<std::shared_ptr<Application>()>> application_registry;

    void registerApplications()
    {
        // Register local apps
        application_registry.emplace(ViewerApplication::getID(), ViewerApplication::getInstance);
        application_registry.emplace(RecorderApplication::getID(), RecorderApplication::getInstance);

        // Plugin applicatins
        satdump::eventBus->fire_event<RegisterApplicationsEvent>({application_registry});
    }
};