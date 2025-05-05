#include "imgui/imgui.h"
#include "sat_finder_ui.h"
#include "core/config.h"
#include "common/tracking/tle.h"
#include "common/geodetic/geodetic_coordinates.h"

namespace satdump
{
    SatFinder::SatFinder() {
        blacklist = {"STARLINK", "ONEWEB", "ORBCOMM"};
        satfinder_thread = std::thread(&SatFinder::satfinder_run, this);

        loadConfig();
    }

    SatFinder::~SatFinder() {
        saveConfig();

        satfinder_should_run = false;
        if (satfinder_thread.joinable())
            satfinder_thread.join();

        if(observer_station)
            predict_destroy_observer(observer_station);
    }

    void SatFinder::satfinder_run() {
        while(satfinder_should_run) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                
                auto time = std::chrono::system_clock::now();
                auto since_epoch = time.time_since_epoch();
                auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
                auto curr_time = millis.count() / 1e3;

                sats_in_sight.clear();

                if(observer_station != nullptr && target_az >= 0 && target_el >= 0) {
                    for(const auto &tle : *general_tle_registry) {
                        if(!containsSubstring(tle.name, blacklist.begin(), blacklist.end())) {
                            predict_orbital_elements_t* satellite_object = predict_parse_tle(tle.line1.c_str(), tle.line2.c_str());
                            if(satellite_object) {
                                predict_position satellite_orbit;
                                predict_observation observation_pos;

                                predict_orbit(satellite_object, &satellite_orbit, predict_to_julian_double(curr_time));
                                predict_observe_orbit(observer_station, &satellite_orbit, &observation_pos);

                                if(std::abs((observation_pos.azimuth * RAD_TO_DEG) - target_az) <= tolerance && std::abs((observation_pos.elevation * RAD_TO_DEG) - target_el) <= tolerance) {
                                    sats_in_sight.emplace_back(Satellite{tle.name, (observation_pos.azimuth * RAD_TO_DEG), (observation_pos.elevation * RAD_TO_DEG), observation_pos.range});
                                }
                                
                                predict_destroy_orbital_elements(satellite_object);
                            }
                        }
                    }
                }
            }
            if(update_period < 0.5)
                update_period = 0.5;
            std::this_thread::sleep_for(std::chrono::milliseconds(uint64_t(update_period * 1e3)));
        }
    }

    void SatFinder::setQTH(double lon, double lat, double alt) {
        std::lock_guard<std::mutex> lock(mutex);
        qth_lon = lon;  
        qth_lat = lat;
        qth_alt = alt;

        if(observer_station)
            predict_destroy_observer(observer_station);
        observer_station = predict_create_observer("Main", qth_lat * DEG_TO_RAD, qth_lon * DEG_TO_RAD, qth_alt);
    }

    void SatFinder::setTarget(double azimuth, double elevation) {
        std::lock_guard<std::mutex> lock(mutex);
        target_az = azimuth;  
        target_el = elevation;    
    }

    void SatFinder::saveConfig()
    {
        config::main_cfg["user"]["satellite_finder"]["update_rate"] = update_period;
        config::main_cfg["user"]["satellite_finder"]["tolerance"] = tolerance;
        config::saveUserConfig();
    }

    void SatFinder::loadConfig()
    {
        if (config::main_cfg["user"].contains("satellite_finder"))
        {
            if (config::main_cfg["user"]["satellite_finder"].contains("update_rate"))
                update_period = config::main_cfg["user"]["satellite_finder"]["update_rate"];
            if (config::main_cfg["user"]["satellite_finder"].contains("tolerance"))
                tolerance = config::main_cfg["user"]["satellite_finder"]["tolerance"];
        }
    }

    void SatFinder::render() {
        std::lock_guard<std::mutex> lock(mutex);

        ImGui::InputDouble("Update Period (s)", &update_period, 0.1, 1, "%.1f", ImGuiInputTextFlags_CharsDecimal);
        if(update_period < 0.5)
                update_period = 0.5;
        ImGui::InputDouble("Az/El Tolerance", &tolerance, 0.1, 1, "%.1f", ImGuiInputTextFlags_CharsDecimal);
        if(tolerance < 0)
            tolerance = 0;

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTable("##satfindergwidgettable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Satellite");
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("Azimuth");
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("Elevation");
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("Distance(km)");

            for(auto &sat : sats_in_sight) {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", sat.name.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.2f", sat.azimuth);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.2f", sat.elevation);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.2f", sat.distance);
            }

            ImGui::EndTable();
        }
    }
}