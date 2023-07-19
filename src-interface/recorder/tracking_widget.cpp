#include "tracking_widget.h"
#include "common/tracking/tle.h"
#include "imgui/imgui.h"
#include "core/module.h"
#include "common/utils.h"
#include "logger.h"
#include "core/config.h"
#include "main_ui.h"

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
            satoptionstr += tle.name + '\0';

        observer_station = predict_create_observer("Main", qth_lat * DEG_TO_RAD, qth_lon * DEG_TO_RAD, qth_alt);

        for (auto &hid : horizons_ids)
            horizonsoptionstr += hid + '\0';

        // Updates on registry updates
        eventBus->register_handler<TLEsUpdatedEvent>([this](TLEsUpdatedEvent)
                                                     {
                                                            tle_update_mutex.lock();

                                                            if (general_tle_registry.size() > 0)
                                                                has_tle = true;

                                                            satoptionstr = "";
                                                            for (auto &tle : general_tle_registry)
                                                                satoptionstr += tle.name + '\0';
                                                                
                                                            tle_update_mutex.unlock(); });
    }

    TrackingWidget::~TrackingWidget()
    {
        predict_destroy_observer(observer_station);
    }

    void TrackingWidget::updateNextPass()
    {
        upcoming_pass_points.clear();

        next_aos_time = 0;
        next_los_time = 0;

        if (horizons_mode)
        {
            if (horizons_data.size() == 0)
                return;

            double timed = getTime();

            int iter = 0;
            for (int i = 0; i < horizons_data.size(); i++)
                if (horizons_data[i].timestamp < timed)
                    iter = i;

            if (horizons_data[iter].el > 0) // Already got AOS
            {
                next_aos_time = timed;

                for (int i = iter - 1; i >= 0; i--) // Attempt to find previous AOS
                {
                    if (horizons_data[i].el <= 0)
                    {
                        next_aos_time = horizons_data[i].timestamp;
                        break;
                    }
                }

                for (int i = iter; i < horizons_data.size(); i++) // Find LOS
                {
                    if (horizons_data[i].el <= 0)
                    {
                        next_los_time = horizons_data[i].timestamp;
                        break;
                    }
                }
            }
            else
            {
                int aos_iter = 0;
                for (int i = iter; i < horizons_data.size(); i++) // Find AOS
                {
                    if (horizons_data[i].el > 0)
                    {
                        next_aos_time = horizons_data[i].timestamp;
                        aos_iter = i;
                        break;
                    }
                }

                if (next_aos_time != 0)
                {
                    for (int i = aos_iter; i < horizons_data.size(); i++) // Find LOS
                    {
                        if (horizons_data[i].el <= 0)
                        {
                            next_los_time = horizons_data[i].timestamp;
                            break;
                        }
                    }
                }
            }

            if (next_aos_time != 0 && next_los_time != 0)
            {
                double time_step = abs(next_los_time - next_aos_time) / 50.0;

                for (double ctime = next_aos_time; ctime <= next_los_time; ctime += time_step)
                {
                    int iter = 0;
                    for (int i = 0; i < horizons_data.size(); i++)
                        if (horizons_data[i].timestamp < ctime)
                            iter = i;

                    upcoming_pass_points.push_back({horizons_data[iter].az, horizons_data[iter].el});
                }
            }
        }
        else
        {
            if (predict_is_geosynchronous(satellite_object))
                return;

            // Get next LOS
            predict_observation next_aos, next_los;
            next_aos = next_los = predict_next_los(observer_station, satellite_object, predict_to_julian_double(getTime()));

            // Calculate the AOS before that LOS
            next_aos_time = next_los_time = predict_from_julian(next_los.time);
            do
            {
                next_aos = predict_next_aos(observer_station, satellite_object, predict_to_julian_double(next_aos_time));
                next_aos_time -= 10;
            } while (predict_from_julian(next_aos.time) >= next_los_time);

            next_los_time = predict_from_julian(next_los.time);
            next_aos_time = predict_from_julian(next_aos.time);

            // Calculate a few points during the pass
            predict_position satellite_orbit2;
            predict_observation observation_pos2;

            double time_step = abs(next_los_time - next_aos_time) / 50.0;

            for (double ctime = next_aos_time; ctime <= next_los_time; ctime += time_step)
            {
                predict_orbit(satellite_object, &satellite_orbit2, predict_to_julian_double(ctime));
                predict_observe_orbit(observer_station, &satellite_orbit2, &observation_pos2);
                upcoming_pass_points.push_back({observation_pos2.azimuth * RAD_TO_DEG, observation_pos2.elevation * RAD_TO_DEG});
            }
        }
    }

    void TrackingWidget::render()
    {
        if (!has_tle)
            return;

        tle_update_mutex.lock();

        float az = 0, el = 0;

        if (horizons_mode)
        {
            if (getTime() > last_horizons_fetch_time + 3600)
                loadHorizons();

            if (horizons_data.size() > 0)
            {
                double timed = getTime();

                int iter = 0;
                for (int i = 0; i < horizons_data.size(); i++)
                    if (horizons_data[i].timestamp < timed)
                        iter = i;

                if (getTime() > next_los_time)
                    updateNextPass();

                az = horizons_data[iter].az;
                el = horizons_data[iter].el;
            }
        }
        else
        {
            if (satellite_object != nullptr)
            {
                predict_orbit(satellite_object, &satellite_orbit, predict_to_julian_double(getTime()));
                predict_observe_orbit(observer_station, &satellite_orbit, &observation_pos);

                if (getTime() > next_los_time)
                    updateNextPass();

                az = observation_pos.azimuth * RAD_TO_DEG;
                el = observation_pos.elevation * RAD_TO_DEG;
            }
        }

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
            }

            if (el > 0)
            {
                float point_x = ImGui::GetCursorScreenPos().x + (d_pplot_size / 2);
                float point_y = ImGui::GetCursorScreenPos().y + (d_pplot_size / 2);

                point_x += sin(az * DEG_TO_RAD) * d_pplot_size * radius * ((90.0 - el) / 90.0);
                point_y -= cos(az * DEG_TO_RAD) * d_pplot_size * radius * ((90.0 - el) / 90.0);

                draw_list->AddCircleFilled({point_x, point_y}, 5 * ui_scale, ImColor(255, 0, 0, 255));
            }

            ImGui::Dummy(ImVec2(d_pplot_size + 3 * ui_scale, d_pplot_size + 3 * ui_scale));
        }

        bool update_global = false;

        ImGui::SetNextItemWidth(ImGui::GetWindowContentRegionWidth());
        if (horizons_mode)
            update_global = update_global || ImGui::Combo("###horizonsselectcombo", &current_horizons, horizonsoptionstr.c_str());
        else
            update_global = update_global || ImGui::Combo("###satelliteselectcombo", &current_satellite, satoptionstr.c_str());

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
                horizons_mode = true;
                update_global = true;
            }
            ImGui::EndTable();
        }

        if (ImGui::BeginTable("##trackingwidgettable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Azimuth");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", az);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Elevation");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%.2f", el);

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

        // Update
        if (update_global)
        {
            if (horizons_mode)
            {
                loadHorizons();
                updateNextPass();
            }
            else
            {
                if (satellite_object != nullptr)
                    predict_destroy_orbital_elements(satellite_object);
                auto &tle = general_tle_registry[current_satellite];
                satellite_object = predict_parse_tle(tle.line1.c_str(), tle.line2.c_str());
                updateNextPass();
            }
        }

        ImGui::Spacing();

        tle_update_mutex.unlock();
    }

    double TrackingWidget::getTime()
    {
        auto time = std::chrono::system_clock::now();
        auto since_epoch = time.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
        return millis.count() / 1e3;
    }

    void TrackingWidget::loadHorizons()
    {
        double curr_time = getTime();
        double start_time = curr_time - 12 * 3600;
        double stop_time = curr_time + 12 * 3600;

        std::string cmd = (std::string) "https://ssd.jpl.nasa.gov/api/horizons.api?format=text" +
                          "&OBJ_DATA=NO" +
                          "&MAKE_EPHEM=YES" +
                          "&COMMAND=" + horizons_ids[current_horizons] +
                          "&CAL_FORMAT=JD" +
                          "&EPHEM_TYPE=OBSERVER" +
                          "&CENTER='coord@399'" +
                          "&COORD_TYPE=GEODETIC" +
                          "&SITE_COORD='" + (qth_lon >= 0 ? "+" : "") + std::to_string(qth_lon) + "," +
                          (qth_lat >= 0 ? "+" : "") + std::to_string(qth_lat) + "," +
                          std::to_string(qth_alt / 1e3) + "'" +
                          "&START_TIME='JD " + std::to_string((start_time / 86400.0) + 2440587.5) + "'" +
                          "&STOP_TIME='JD " + std::to_string((stop_time / 86400.0) + 2440587.5) + "'" +
                          "&STEP_SIZE='8640'" + // 86400
                          "&QUANTITIES='4,20'";

        std::string req_result;

        int err = perform_http_request(cmd, req_result);

        if (err != 0)
        {
            logger->error("Could not fetch data from Horizons!");
            return;
        }

        last_horizons_fetch_time = curr_time;

        std::istringstream req_results(req_result);
        std::string line;

        horizons_data.clear();

        bool fount_soe = false;
        bool fount_eoe = false;
        while (getline(req_results, line))
        {
            if (!fount_soe)
            {
                if (line.find("$$SOE") != std::string::npos)
                    fount_soe = true;
                continue;
            }

            if (fount_eoe)
            {
                continue;
            }
            else
            {
                if (line.find("$$EOE") != std::string::npos)
                    fount_eoe = true;
            }

            double julian_time = 0;
            double az = 0;
            double el = 0;
            double delta = 0;
            double deldot = 0;

            if (sscanf(line.c_str(), "%lf%*s    %lf %lf %lf  %lf  %lf",
                       &julian_time, &az, &el, &delta, &deldot) == 5 ||
                sscanf(line.c_str(), "%lf    %lf %lf %lf  %lf  %lf",
                       &julian_time, &az, &el, &delta, &deldot) == 5)
            {
                double ctime = (julian_time - 2440587.5) * 86400.0;
                // logger->info("%s %f %f", timestamp_to_string(ctime).c_str(), az, el);
                horizons_data.push_back({ctime, (float)az, (float)el});
            }
        }
    }

}
