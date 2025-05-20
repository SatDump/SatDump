#include "tracking.h"
#include "common/tracking/tle.h"
#include "common/utils.h"
#include "imgui/implot3d/implot3d.h"

namespace satdump
{
    WipTrackingHandler::WipTrackingHandler()
    {
        handler_tree_icon = u8"\uf471";

        auto tle = general_tle_registry->get_from_norad(40069);
        satellite_object = predict_parse_tle(tle->line1.c_str(), tle->line2.c_str());
    }

    WipTrackingHandler::~WipTrackingHandler() {}

    void WipTrackingHandler::drawMenu()
    {
        if (ImGui::CollapsingHeader("Tracking", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ////
        }
    }

    void WipTrackingHandler::drawMenuBar() {}

    void WipTrackingHandler::drawContents(ImVec2 win_size)
    {
        double utc_time = getTime();
        predict_orbit(satellite_object, &satellite_orbit, predict_to_julian_double(utc_time));

        ImPlot3D::BeginPlot("ObjectsTest");

        double x = 0, y = 0, z = 0;
        ImPlot3D::PlotScatter("Earth", &x, &y, &z, 1);

        x = satellite_orbit.position[0];
        y = satellite_orbit.position[1];
        z = satellite_orbit.position[2];

        ImPlot3D::PlotScatter("METEOR-M 2", &x, &y, &z, 1);

        ImPlot3D::EndPlot();
    }
} // namespace satdump
