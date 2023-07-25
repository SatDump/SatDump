#include "tracking_widget.h"
#include "common/utils.h"
#include "logger.h"
#include "imgui/imgui.h"

namespace satdump
{
    void TrackingWidget::renderConfigWindow()
    {
        if (show_window_config)
        {
            ImGui::Begin("Tracking Configuration", &show_window_config);

            if (ImGui::CollapsingHeader("Rotator"))
            {
                ImGui::InputFloat("Update Period (s)", &rotator_update_period);
            }

            ImGui::End();
        }
    }
}
