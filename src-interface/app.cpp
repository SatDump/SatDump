#include "app.h"
#include "imgui/imgui.h"

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
};