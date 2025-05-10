#include "tracking.h"

namespace satdump
{
    WipTrackingHandler::WipTrackingHandler() { handler_tree_icon = u8"\uf471"; }

    WipTrackingHandler::~WipTrackingHandler() {}

    void WipTrackingHandler::drawMenu()
    {
        if (ImGui::CollapsingHeader("Tracking", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ////
        }
    }

    void WipTrackingHandler::drawMenuBar() {}

    void WipTrackingHandler::drawContents(ImVec2 win_size) {}
} // namespace satdump
