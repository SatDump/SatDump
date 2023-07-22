#include "tracking_widget.h"
#include "common/tracking/tle.h"
#include "imgui/imgui.h"
#include "core/module.h"
#include "core/config.h"
#include "main_ui.h"

#include "rotcl_handler.h"

namespace satdump
{
    TrackingWidget::TrackingWidget()
    {
        try
        {
            qth_lon = config::main_cfg["satdump_general"]["qth_lon"]["value"].get<double>();
            qth_lat = config::main_cfg["satdump_general"]["qth_lat"]["value"].get<double>();
            qth_alt = config::main_cfg["satdump_general"]["qth_alt"]["value"].get<double>();
        }
        catch (std::exception &e)
        {
            logger->error("Could not get QTH lon/lat! %s", e.what());
        }

        logger->trace("Using QTH %f %f Alt %f", qth_lon, qth_lat, qth_alt);

        if (general_tle_registry.size() > 0)
            has_tle = true;

        for (auto &tle : general_tle_registry)
            satoptions.push_back(tle.name);

        observer_station = predict_create_observer("Main", qth_lat * DEG_TO_RAD, qth_lon * DEG_TO_RAD, qth_alt);

        // Updates on registry updates
        eventBus->register_handler<TLEsUpdatedEvent>([this](TLEsUpdatedEvent)
                                                     {
                                                            tle_update_mutex.lock();

                                                            if (general_tle_registry.size() > 0)
                                                                has_tle = true;

                                                            satoptions.clear();
                                                            for (auto &tle : general_tle_registry)
                                                                satoptions.push_back(tle.name);
                                                                
                                                            tle_update_mutex.unlock(); });

        backend_thread = std::thread(&TrackingWidget::backend_run, this);

        rotator_handler = std::make_shared<RotctlHandler>();
    }

    TrackingWidget::~TrackingWidget()
    {
        predict_destroy_observer(observer_station);

        backend_should_run = false;
        if (backend_thread.joinable())
            backend_thread.join();
    }

    void TrackingWidget::render()
    {
        if (!has_tle)
            return;

        // tle_update_mutex.lock();

        {
            int d_pplot_size = ImGui::GetWindowContentRegionWidth();
            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                     ImVec2(ImGui::GetCursorScreenPos().x + d_pplot_size, ImGui::GetCursorScreenPos().y + d_pplot_size),
                                     light_theme ? ImColor(255, 255, 255, 255) : ImColor::HSV(0, 0, 0));

            float radius = 0.45;
            float radius1 = d_pplot_size * radius * (3.0 / 9.0);
            float radius2 = d_pplot_size * radius * (6.0 / 9.0);
            float radius3 = d_pplot_size * radius * (9.0 / 9.0);

            draw_list->AddCircle({ImGui::GetCursorScreenPos().x + (d_pplot_size / 2),
                                  ImGui::GetCursorScreenPos().y + (d_pplot_size / 2)},
                                 radius1, ImColor(0, 255, 0, 255), 0, 2);
            draw_list->AddCircle({ImGui::GetCursorScreenPos().x + (d_pplot_size / 2),
                                  ImGui::GetCursorScreenPos().y + (d_pplot_size / 2)},
                                 radius2, ImColor(0, 255, 0, 255), 0, 2);
            draw_list->AddCircle({ImGui::GetCursorScreenPos().x + (d_pplot_size / 2),
                                  ImGui::GetCursorScreenPos().y + (d_pplot_size / 2)},
                                 radius3, ImColor(0, 255, 0, 255), 0, 2);

            draw_list->AddLine({ImGui::GetCursorScreenPos().x + (d_pplot_size / 2),
                                ImGui::GetCursorScreenPos().y},
                               {ImGui::GetCursorScreenPos().x + (d_pplot_size / 2),
                                ImGui::GetCursorScreenPos().y + d_pplot_size},
                               ImColor(0, 255, 0, 255), 2);
            draw_list->AddLine({ImGui::GetCursorScreenPos().x,
                                ImGui::GetCursorScreenPos().y + (d_pplot_size / 2)},
                               {ImGui::GetCursorScreenPos().x + d_pplot_size,
                                ImGui::GetCursorScreenPos().y + (d_pplot_size / 2)},
                               ImColor(0, 255, 0, 255), 2);

            if (upcoming_pass_points.size() > 1)
            {
                upcoming_passes_mtx.lock();
                for (int i = 0; i < (int)upcoming_pass_points.size() - 1; i++)
                {
                    auto &p1 = upcoming_pass_points[i];
                    auto &p2 = upcoming_pass_points[i + 1];

                    float point_x1, point_x2, point_y1, point_y2;
                    point_x1 = point_x2 = ImGui::GetCursorScreenPos().x + (d_pplot_size / 2);
                    point_y1 = point_y2 = ImGui::GetCursorScreenPos().y + (d_pplot_size / 2);

                    point_x1 += sin(p1.first * DEG_TO_RAD) * d_pplot_size * radius * ((90.0 - p1.second) / 90.0);
                    point_y1 -= cos(p1.first * DEG_TO_RAD) * d_pplot_size * radius * ((90.0 - p1.second) / 90.0);

                    point_x2 += sin(p2.first * DEG_TO_RAD) * d_pplot_size * radius * ((90.0 - p2.second) / 90.0);
                    point_y2 -= cos(p2.first * DEG_TO_RAD) * d_pplot_size * radius * ((90.0 - p2.second) / 90.0);

                    draw_list->AddLine({point_x1, point_y1},
                                       {point_x2, point_y2},
                                       ImColor(255, 165, 0, 255), 2.0);
                }
                upcoming_passes_mtx.unlock();
            }

            if (current_el > 0)
            {
                float point_x = ImGui::GetCursorScreenPos().x + (d_pplot_size / 2);
                float point_y = ImGui::GetCursorScreenPos().y + (d_pplot_size / 2);

                point_x += sin(current_az * DEG_TO_RAD) * d_pplot_size * radius * ((90.0 - current_el) / 90.0);
                point_y -= cos(current_az * DEG_TO_RAD) * d_pplot_size * radius * ((90.0 - current_el) / 90.0);

                draw_list->AddCircleFilled({point_x, point_y}, 5 * ui_scale, ImColor(255, 0, 0, 255));
            }

            if (rotator_handler->is_connected())
            {
                {
                    float point_x = ImGui::GetCursorScreenPos().x + (d_pplot_size / 2);
                    float point_y = ImGui::GetCursorScreenPos().y + (d_pplot_size / 2);

                    point_x += sin(current_rotator_az * DEG_TO_RAD) * d_pplot_size * radius * ((90.0 - current_rotator_el) / 90.0);
                    point_y -= cos(current_rotator_az * DEG_TO_RAD) * d_pplot_size * radius * ((90.0 - current_rotator_el) / 90.0);

                    draw_list->AddCircle({point_x, point_y}, 9 * ui_scale, ImColor(0, 237, 255, 255), 0, 2.0);
                }

                if (rotator_engaged)
                {
                    float point_x = ImGui::GetCursorScreenPos().x + (d_pplot_size / 2);
                    float point_y = ImGui::GetCursorScreenPos().y + (d_pplot_size / 2);

                    point_x += sin(current_req_rotator_az * DEG_TO_RAD) * d_pplot_size * radius * ((90.0 - current_req_rotator_el) / 90.0);
                    point_y -= cos(current_req_rotator_az * DEG_TO_RAD) * d_pplot_size * radius * ((90.0 - current_req_rotator_el) / 90.0);

                    draw_list->AddLine({point_x - 5 * ui_scale, point_y}, {point_x - 12 * ui_scale, point_y}, ImColor(0, 237, 255, 255), 2.0);
                    draw_list->AddLine({point_x + 5 * ui_scale, point_y}, {point_x + 12 * ui_scale, point_y}, ImColor(0, 237, 255, 255), 2.0);
                    draw_list->AddLine({point_x, point_y - 5 * ui_scale}, {point_x, point_y - 12 * ui_scale}, ImColor(0, 237, 255, 255), 2.0);
                    draw_list->AddLine({point_x, point_y + 5 * ui_scale}, {point_x, point_y + 12 * ui_scale}, ImColor(0, 237, 255, 255), 2.0);
                }
            }

            ImGui::Dummy(ImVec2(d_pplot_size + 3 * ui_scale, d_pplot_size + 3 * ui_scale));
        }

        bool update_global = false;

        if (backend_needs_update)
            style::beginDisabled();

        ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth());
        if (horizons_mode)
        {
            if (ImGui::BeginCombo("###horizonsselectcombo", horizonsoptions[current_horizons].second.c_str()))
            {
                ImGui::InputTextWithHint("##horizonssatellitetracking", u8"\uf422   Search", &horizonssearchstr);
                for (int i = 0; i < (int)horizonsoptions.size(); i++)
                {
                    bool show = true;
                    if (horizonssearchstr.size() != 0)
                        show = isStringPresent(horizonsoptions[i].second, horizonssearchstr);
                    if (show)
                    {
                        if (ImGui::Selectable(horizonsoptions[i].second.c_str(), i == current_horizons))
                        {
                            current_horizons = i;
                            update_global = true;
                        }
                    }
                }
                ImGui::EndCombo();
            }
        }
        else
        {
            if (ImGui::BeginCombo("###satelliteselectcombo", satoptions[current_satellite].c_str()))
            {
                ImGui::InputTextWithHint("##searchsatellitetracking", u8"\uf422   Search", &satsearchstr);
                for (int i = 0; i < (int)satoptions.size(); i++)
                {
                    bool show = true;
                    if (satsearchstr.size() != 0)
                        show = isStringPresent(satoptions[i], satsearchstr);
                    if (show)
                    {
                        if (ImGui::Selectable(satoptions[i].c_str(), i == current_satellite))
                        {
                            current_satellite = i;
                            update_global = true;
                        }
                    }
                }
                ImGui::EndCombo();
            }
        }

        if (ImGui::BeginTable("##trackingradiotable", 2, NULL))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            if (ImGui::RadioButton("Satellites", !horizons_mode))
            {
                horizons_mode = false;
                update_global = true;
            }
            ImGui::TableSetColumnIndex(1);
            if (ImGui::RadioButton("Horizons", horizons_mode))
            {
                if (horizonsoptions.size() == 1)
                    horizonsoptions = pullHorizonsList();
                horizons_mode = true;
                update_global = true;
            }
            ImGui::EndTable();
        }

        if (backend_needs_update)
            style::endDisabled();

        if (update_global)
            backend_needs_update = true;

        if (ImGui::BeginTable("##trackingwidgettable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Azimuth");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.3f", current_az);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Elevation");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.3f", current_el);

            if (next_aos_time != 0 && next_los_time != 0)
            {
                double timeOffset = 0, ctime = getTime();
                if (next_aos_time > ctime)
                    timeOffset = next_aos_time - ctime;
                else
                    timeOffset = next_los_time - ctime;

                int hours = timeOffset / 3600;
                int minutes = fmod(timeOffset / 60, 60);
                int seconds = fmod(timeOffset, 60);

                std::string time_dis = (hours < 10 ? "0" : "") + std::to_string(hours) + ":" +
                                       (minutes < 10 ? "0" : "") + std::to_string(minutes) + ":" +
                                       (seconds < 10 ? "0" : "") + std::to_string(seconds);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Next Event");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", time_dis.c_str());
            }

#if 1
            if (!horizons_mode && satellite_object != nullptr)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Azimuth Rate");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.2f", observation_pos.azimuth_rate * RAD_TO_DEG);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Elevation Rate");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.2f", observation_pos.elevation_rate * RAD_TO_DEG);
            }
#endif

            ImGui::EndTable();
        }

        ImGui::Spacing();

        //////////////////

        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTable("##trackingwidgettable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Rot Az");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Rot El");

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::InputFloat("##Rot Az", &current_req_rotator_az);
            ImGui::TableSetColumnIndex(1);
            ImGui::InputFloat("##Rot El", &current_req_rotator_el);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%.3f", current_rotator_az);
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.3f", current_rotator_el);

            ImGui::EndTable();
        }

        ImGui::Checkbox("Engage", &rotator_engaged);
        ImGui::SameLine();
        ImGui::Checkbox("Track", &rotator_tracking);

        ImGui::Separator();

        if (rotator_handler->is_connected())
            style::beginDisabled();
        if (ImGui::Combo("Type##rotatortype", &selected_rotator_handler, "Rotctl\0"
                                                                         "PstRotator (Untested)\0"))
        {
            rotator_handler_mtx.lock();
            if (selected_rotator_handler == 0)
                rotator_handler = std::make_shared<RotctlHandler>();
            rotator_handler_mtx.unlock();
        }
        if (rotator_handler->is_connected())
            style::endDisabled();

        rotator_handler->render();

        ImGui::Spacing();

        // tle_update_mutex.unlock();
    }

    double TrackingWidget::getTime()
    {
        auto time = std::chrono::system_clock::now();
        auto since_epoch = time.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
        return millis.count() / 1e3;
    }

}
