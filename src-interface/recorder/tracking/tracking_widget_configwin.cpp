#include "tracking_widget.h"
#include "common/utils.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/imgui_utils.h"

namespace satdump
{
    void TrackingWidget::renderConfigWindow()
    {
        if (show_window_config)
        {
            ImGui::Begin("Tracking Configuration", &show_window_config);
            ImGui::SetWindowSize(ImVec2(800, 550), ImGuiCond_FirstUseEver);

            if (ImGui::BeginTabBar("##trackingtabbar"))
            {
                if (ImGui::BeginTabItem("Scheduling"))
                {
                    ImGui::BeginChild("##trackingbarschedule", ImVec2(0, 0), false, ImGuiWindowFlags_NoResize);

                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Rotator Config"))
                {
                    ImGui::InputFloat("Update Period (s)", &rotator_update_period);
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }

            if (config_window_was_asked)
                ImGuiUtils_BringCurrentWindowToFront();
            config_window_was_asked = false;

            ImGui::End();
        }
    }
}
