#include "object_tracker.h"
#include "common/geodetic/geodetic_coordinates.h"
#include "common/utils.h"
#include "logger.h"
#include "common/tracking/tle.h"
#include <cfloat>
#include "imgui/imgui.h"
#include "core/style.h"

#include "common/widgets/azel_input.h"

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
                            if (meridian_flip_correction)
                                rot_current_req_pos.az = correctRotatorAzimuth(sat_current_pos.az);
                        }
                        else if (rotator_park_while_idle)
                        {
                            if (getTime() + rotator_unpark_at_minus > next_aos_time)
                            {
                                rot_current_req_pos.az = sat_next_aos_pos.az;
                                rot_current_req_pos.el = sat_next_aos_pos.el;
                                if (meridian_flip_correction)
                                    rot_current_req_pos.az = correctRotatorAzimuth(sat_next_aos_pos.az);
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

                    // if (rot_current_req_pos.el < 0)
                    //     rot_current_req_pos.el = 0;

                    if (rot_current_reqlast_pos.az != rot_current_req_pos.az || rot_current_reqlast_pos.el != rot_current_req_pos.el)
                        if (rotator_handler->set_pos(rot_current_req_pos.az, rot_current_req_pos.el) != rotator::ROT_ERROR_OK)
                            logger->error("Error setting rotator position %f %f!", rot_current_req_pos.az, rot_current_req_pos.el);

                    rot_current_reqlast_pos.az = rot_current_req_pos.az;
                    rot_current_reqlast_pos.el = rot_current_req_pos.el;

                    if(rotator_target_pos_updated_callback) {
                        rotator_target_pos_updated_callback(rot_current_req_pos.az, rot_current_req_pos.el);
                    }
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

    float ObjectTracker::correctRotatorAzimuth(const float az)
    {
        float resultAzimuth = az;
        if (rotator_az_max - rotator_az_min <= 360)
        {
            // Rotator doesn't support more than 360°
            return resultAzimuth;
        }
        if (northbound_cross || southbound_cross)
        {
            if (southbound_cross)
            {
                if (sat_next_aos_pos.az < 90)
                {
                    // Pass start from east
                    if (resultAzimuth <= 90)
                    {
                        // We are currently in the easter region
                        resultAzimuth += 360;
                        if (resultAzimuth > rotator_az_max)
                        {
                            resultAzimuth = az;
                        }
                    }
                }
                else
                {
                    // Pass start from west
                    if (resultAzimuth >= 270)
                    {
                        // We are currently in the western region
                        resultAzimuth -= 360;
                        if (resultAzimuth < rotator_az_min)
                        {
                            resultAzimuth = az;
                        }
                    }
                }
            }
            if (northbound_cross)
            {
                if (sat_next_los_pos.az < 90)
                {
                    // Pass end on east
                    if (az <= 90)
                    {
                        // We are currently in the easter region
                        resultAzimuth += 360;
                        if (resultAzimuth > rotator_az_max)
                        {
                            resultAzimuth = az;
                        }
                    }
                }
                else
                {
                    // Pass end on west
                    if (resultAzimuth >= 270)
                    {
                        // We are currently in the western region
                        resultAzimuth -= 360;
                        if (resultAzimuth < rotator_az_min)
                        {
                            resultAzimuth = az;
                        }
                    }
                }
            }
        }
        return resultAzimuth;
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
            widgets::AzElInput("##reqaz", &rot_current_req_pos.az, !rotator_tracking);
            ImGui::TableSetColumnIndex(1);
            widgets::AzElInput("##reqel", &rot_current_req_pos.el, !rotator_tracking);

            if (rotator_arrowkeys_enable && !rotator_tracking)
            {
                if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
                    rot_current_req_pos.el -= rotator_arrowkeys_increment;
                if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
                    rot_current_req_pos.el += rotator_arrowkeys_increment;
                if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
                    rot_current_req_pos.az += rotator_arrowkeys_increment;
                if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
                    rot_current_req_pos.az -= rotator_arrowkeys_increment;
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            widgets::AzElInput("##curaz", &rot_current_pos.az, false);
            ImGui::TableSetColumnIndex(1);
            widgets::AzElInput("##curel", &rot_current_pos.el, false);

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

        ImGui::Checkbox("Meridian flip correction", &meridian_flip_correction);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("By enabling this setting, you can allow your rotator to go beyond the default 0-360° azimuth range");

        if (meridian_flip_correction)
        {
            ImGui::InputInt("Minimum Azimuth", &rotator_az_min);
            ImGui::InputInt("Maximum Azimuth", &rotator_az_max);
        }

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

        ImGui::Checkbox("Arrow Keys Control", &rotator_arrowkeys_enable);
        if (rotator_arrowkeys_enable)
            ImGui::InputDouble("Arrow Keys Control Increment", &rotator_arrowkeys_increment);
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
        v["meridian_flip_correction"] = meridian_flip_correction;
        v["rotator_az_min"] = rotator_az_min;
        v["rotator_az_max"] = rotator_az_max;
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
        meridian_flip_correction = getValueOrDefault(v["meridian_flip_correction"], meridian_flip_correction);
        rotator_az_min = getValueOrDefault(v["rotator_az_min"], rotator_az_min);
        rotator_az_max = getValueOrDefault(v["rotator_az_max"], rotator_az_max);
    }
}
