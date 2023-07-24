#pragma once

#include "libs/predict/predict.h"
#include "common/geodetic/geodetic_coordinates.h"
#include <vector>
#include <mutex>
#include <thread>
#include <memory>

#include "rotator_handler.h"

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

        std::vector<std::string> satoptions;
        int current_satellite = 0;
        std::string satsearchstr;

        double getTime();

        double last_horizons_fetch_time = 0;

        struct HorizonsV
        {
            double timestamp;
            float az, el;
        };

        std::vector<std::pair<int, std::string>> horizonsoptions = {{-234, "STEREO-A"}};
        int current_horizons = 0;
        std::vector<HorizonsV> horizons_data;
        std::string horizonssearchstr;

        std::vector<std::pair<int, std::string>> pullHorizonsList();

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

        double next_aos_time, next_los_time;

        float next_aos_az = 0, next_aos_el = 0;

    private: // Rotator control
        bool rotatorth_should_run = true;
        std::thread rotatorth_thread;
        void rotatorth_run();

        bool rotator_engaged = false;
        bool rotator_tracking = false;
        float current_rotator_az = 0, current_rotator_el = 0;
        float current_req_rotator_az = 0, current_req_rotator_el = 0;

        int selected_rotator_handler = 0;
        std::mutex rotator_handler_mtx;
        std::shared_ptr<RotatorHandler> rotator_handler;

        bool show_rotator_config = false;
        float rotator_update_period = 1;

    public:
        TrackingWidget();
        ~TrackingWidget();

        void render();
    };
}
