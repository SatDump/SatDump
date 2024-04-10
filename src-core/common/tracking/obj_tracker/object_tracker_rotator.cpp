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
                            if (!rotator_rounding)
                            {
                                rot_current_req_pos.az = sat_current_pos.az;
                                rot_current_req_pos.el = sat_current_pos.el;
                            }
                            else
                            {
                                rot_current_req_pos.az = (round(sat_current_pos.az * rotator_decimal_multiplier)) / rotator_decimal_multiplier;
                                rot_current_req_pos.el = (round(sat_current_pos.el * rotator_decimal_multiplier)) / rotator_decimal_multiplier;
                            }
                        }
                        else if (rotator_park_while_idle)
                        {
                            if (getTime() + rotator_unpark_at_minus > next_aos_time)
                            {
                                rot_current_req_pos.az = sat_next_aos_pos.az;
                                rot_current_req_pos.el = sat_next_aos_pos.el;
                            }
                            else
                            {
                                rot_current_req_pos.az = rotator_park_position.az;
                                rot_current_req_pos.el = rotator_park_position.el;
                            }
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

    void ObjectTracker::renderRotatorConfig()
    {
        ImGui::InputDouble("Update Period (s)", &rotator_update_period);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Checkbox("Park while idle", &rotator_park_while_idle);

        if (rotator_park_while_idle)
        {
            ImGui::InputFloat("Park Az##Rot Az", &rotator_park_position.az);
            ImGui::InputFloat("Park El##Rot El", &rotator_park_position.el);
            ImGui::InputDouble("Unpark Time##Rot Unpark Time", &rotator_unpark_at_minus);
        }

        ImGui::Checkbox("AZ EL Decimal rounding", &rotator_rounding);

        if (rotator_rounding && ImGui::InputInt("Decimal Place Precision", &rotator_decimal_precision, 1, 3))
        {
            if (rotator_decimal_precision > 3)
            {
                rotator_decimal_precision = 3;
            }
            else if (rotator_decimal_precision == 0)
            {
                rotator_decimal_precision = 1;
            }
            switch (rotator_decimal_precision)
            {
            case 1:
                rotator_decimal_multiplier = 10;
                break;
            case 2:
                rotator_decimal_multiplier = 100;
                break;
            case 3:
                rotator_decimal_multiplier = 1000;
                break;
            }
        }
    }

    nlohmann::json ObjectTracker::getRotatorConfig()
    {
        nlohmann::json v;
        v["update_period"] = rotator_update_period;
        v["park_while_idle"] = rotator_park_while_idle;
        v["park_position"] = rotator_park_position;
        v["unpark_at_minus"] = rotator_unpark_at_minus;
        v["rounding"] = rotator_rounding;
        v["rounding_decimal_places"] = rotator_decimal_precision;
        return v;
    }

    void ObjectTracker::setRotatorConfig(nlohmann::json v)
    {
        rotator_update_period = getValueOrDefault(v["update_period"], rotator_update_period);
        rotator_park_while_idle = getValueOrDefault(v["park_while_idle"], rotator_park_while_idle);
        rotator_park_position = getValueOrDefault(v["park_position"], rotator_park_position);
        rotator_unpark_at_minus = getValueOrDefault(v["unpark_at_minus"], rotator_unpark_at_minus);
        rotator_rounding = getValueOrDefault(v["rounding"], rotator_rounding);
        rotator_decimal_precision = getValueOrDefault(v["rotator_decimal_places"], rotator_decimal_precision);
    }
}
