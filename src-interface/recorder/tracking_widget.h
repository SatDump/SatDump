#pragma once

#include "libs/predict/predict.h"
#include "common/geodetic/geodetic_coordinates.h"
#include <vector>
#include <mutex>
#include <thread>

#include "rotcl_client.h"

namespace satdump
{
    class TrackingWidget
    {
    private:
        double qth_lon = 0;
        double qth_lat = 0;
        double qth_alt = 0;

        bool has_tle = false;

        bool horizons_mode = false;

        predict_orbital_elements_t *satellite_object = nullptr;
        predict_position satellite_orbit;
        predict_observer_t *observer_station;
        predict_observation observation_pos;
        double next_aos_time, next_los_time;

        std::string satoptionstr;
        int current_satellite = 0;

        double getTime();

        double last_horizons_fetch_time = 0;

        struct HorizonsV
        {
            double timestamp;
            float az, el;
        };

        std::vector<std::string> horizons_ids = {"STEREO-A", "STEREO-B", "JUICE", "JWST", "SOHO", "Sun", "CH-3", "New Horizons"};
        std::string horizonsoptionstr;
        int current_horizons = 0;
        std::vector<HorizonsV> horizons_data;

        void loadHorizons();

        std::mutex tle_update_mutex;

    private: // Core
        bool backend_should_run = true;
        std::thread backend_thread;
        void backend_run();
        bool backend_needs_update = true;

    private: // Current satellite
        float current_az = 0, current_el = 0;

        std::mutex upcoming_passes_mtx;
        std::vector<std::pair<float, float>> upcoming_pass_points;
        void updateNextPass();

    private: // Rotator control
        bool rotator_engaged = false;
        bool rotator_tracking = false;
        float current_rotator_az = 0, current_rotator_el = 0;
        float current_req_rotator_az = 0, current_req_rotator_el = 0;
        void updateRotator();

        RotclClient rotctld_client;

    public:
        TrackingWidget();
        ~TrackingWidget();

        void render();
    };
}
