#pragma once

#include "libs/predict/predict.h"
#include "common/geodetic/geodetic_coordinates.h"
#include <vector>
#include <mutex>
#include <thread>
#include <memory>
#include <functional>

#include "rotator_handler.h"

namespace satdump
{
    class TrackingWidget
    {
    private: // QTH Config
        double qth_lon = 0;
        double qth_lat = 0;
        double qth_alt = 0;

    private: // General configs/utils
        bool has_tle = false;
        bool horizons_mode = false;

        double getTime();

    private: // Satellite Tracking (libpredict)
        std::vector<std::string> satoptions;
        int current_satellite = 0;
        std::string satsearchstr;

        predict_orbital_elements_t *satellite_object = nullptr;
        predict_position satellite_orbit;
        predict_observer_t *observer_station;
        predict_observation observation_pos;

        std::mutex tle_update_mutex;

    private: // Horizons Tracking (horizons)
        struct HorizonsV
        {
            double timestamp;
            float az, el;
        };

        std::vector<std::pair<int, std::string>> horizonsoptions = {{-234, "STEREO-A"}};
        int current_horizons = 0;
        std::vector<HorizonsV> horizons_data;
        std::string horizonssearchstr;

        double last_horizons_fetch_time = 0;

        std::vector<std::pair<int, std::string>> pullHorizonsList();
        void loadHorizons();
        std::vector<HorizonsV> pullHorizonsData(double start_time, double stop_time, int num_points);

    private: // Core
        bool backend_should_run = true;
        std::thread backend_thread;
        void backend_run();
        bool backend_needs_update = true;

    private: // Config Window
        bool show_window_config = false;

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

        float rotator_update_period = 1;

    public: // Autotrack (Basic for now, satellites only, Horizons to be done...)
        struct SatellitePass
        {
            int norad;
            double aos_time;
            double los_time;
            float max_elevation;
        };

        struct TrackedObject
        {
            int norad;

            // Config
            double frequency = 100e6;
            bool record = false;
            bool live = false;
            std::string pipeline_name = "";
        };

    private:
        int tracking_sats_menu_selected_1 = 0, tracking_sats_menu_selected_2 = 0;
        std::vector<TrackedObject> enabled_satellites;

        std::mutex upcoming_satellite_passes_mtx;
        std::vector<SatellitePass> upcoming_satellite_passes_all;
        std::vector<SatellitePass> upcoming_satellite_passes_sel;

        bool autotrack_engaged = false;

        void processAutotrack();
        void updateAutotrackPasses();
        void renderAutotrackConfig();

        bool autotrack_pass_has_started = false;

    private: // UI Functions
        void renderPolarPlot();
        void renderSelectionMenu();
        void renderObjectStatus();
        void renderRotatorStatus();
        void renderConfigWindow();

    public: // Handlers
        std::function<void(SatellitePass, TrackedObject)> aos_callback = [](SatellitePass, TrackedObject) {};
        std::function<void(SatellitePass, TrackedObject)> los_callback = [](SatellitePass, TrackedObject) {};

    public:
        TrackingWidget();
        ~TrackingWidget();

        void render();
    };
}
