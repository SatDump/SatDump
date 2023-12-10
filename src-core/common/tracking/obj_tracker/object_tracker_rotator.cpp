#include "object_tracker.h"
#include "common/geodetic/geodetic_coordinates.h"
#include "common/utils.h"
#include "logger.h"
#include "common/tracking/tle.h"
#include <cfloat>
#include "imgui/imgui.h"
#include "core/style.h"

namespace satdump
{
    void ObjectTracker::rotatorth_run()
    {
        while (rotatorth_should_run)
        {
            rotator_handler_mtx.lock();
            if (rotator_handler && rotator_handler->is_connected())
            {
                // logger->info("Rot update!");

                if (rotator_handler->get_pos(&rot_current_pos.az, &rot_current_pos.el) != rotator::ROT_ERROR_OK)
                    logger->error("Error getting rotator position!");

                if (rotator_engaged)
                {
                    if (rotator_tracking)
                    {
                        if (sat_current_pos.el > 0)
                        {
                            rot_current_req_pos.az = sat_current_pos.az;
                            rot_current_req_pos.el = sat_current_pos.el;
                        }
                        else
                        {
                            rot_current_req_pos.az = sat_next_aos_pos.az;
                            rot_current_req_pos.el = sat_next_aos_pos.el;
                        }
                    }

                    if (rot_current_req_pos.el < 0)
                        rot_current_req_pos.el = 0;

                    if (rot_current_reqlast_pos.az != rot_current_req_pos.az || rot_current_reqlast_pos.el != rot_current_req_pos.el)
                        if (rotator_handler->set_pos(rot_current_req_pos.az, rot_current_req_pos.el) != rotator::ROT_ERROR_OK)
                            logger->error("Error setting rotator position %f %f!", rot_current_req_pos.az, rot_current_req_pos.el);

                    rot_current_reqlast_pos.az = rot_current_req_pos.az;
                    rot_current_reqlast_pos.el = rot_current_req_pos.el;
                }

                double rotator_update_period = 0.1;

                std::this_thread::sleep_for(std::chrono::milliseconds(uint64_t(rotator_update_period * 1e3)));
            }
            else
            {
                rotator_handler_mtx.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                rotator_handler_mtx.lock();
            }
            rotator_handler_mtx.unlock();
        }
    }

    void ObjectTracker::renderRotatorStatus()
    {
        if (!rotator_handler)
            return;

        if (ImGui::BeginTable("##trackingwidgettable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Rotator Az");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Rotator El");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::InputFloat("##Rot Az", &rot_current_req_pos.az);
            ImGui::TableSetColumnIndex(1);
            ImGui::InputFloat("##Rot El", &rot_current_req_pos.el);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%.3f", rot_current_pos.az);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.3f", rot_current_pos.el);

            ImGui::EndTable();
        }

        ImGui::Checkbox("Engage", &rotator_engaged);
        ImGui::SameLine();
        ImGui::Checkbox("Track", &rotator_tracking);
    }
}
