#pragma once

#include "libs/predict/predict.h"
#include "common/geodetic/geodetic_coordinates.h"
#include <vector>
#include <mutex>
#include <thread>
#include <memory>
#include <functional>
#include "core/config.h"
#include "nlohmann/json_utils.h"

#include "common/tracking/obj_tracker/object_tracker.h"
#include "common/tracking/scheduler/scheduler.h"

namespace satdump
{
    class TrackingWidget
    {
    private: // QTH Config
        double qth_lon = 0;
        double qth_lat = 0;
        double qth_alt = 0;

    public: // Handlers
        std::function<void(AutoTrackCfg, SatellitePass, TrackedObject)> aos_callback = [](AutoTrackCfg, SatellitePass, TrackedObject) {};
        std::function<void(AutoTrackCfg, SatellitePass, TrackedObject)> los_callback = [](AutoTrackCfg, SatellitePass, TrackedObject) {};

    private:
        ObjectTracker object_tracker = ObjectTracker(true);
        AutoTrackScheduler auto_scheduler;

        std::shared_ptr<rotator::RotatorHandler> rotator_handler;
        std::vector<rotator::RotatorHandlerOption> rotator_options;
        std::string rotator_options_str;
        int selected_rotator_handler = 0;

        bool config_window_was_asked = false, show_window_config = false;

    private:
        void saveConfig()
        {
            config::main_cfg["user"]["recorder_tracking"]["enabled_objects"] = auto_scheduler.getTracked();
            config::main_cfg["user"]["recorder_tracking"]["rotator_algo"] = object_tracker.getRotatorConfig();
            config::main_cfg["user"]["recorder_tracking"]["autotrack_cfg"] = auto_scheduler.getAutoTrackCfg();
            if (rotator_handler)
                config::main_cfg["user"]["recorder_tracking"]["rotator_config"][rotator_handler->get_id()] = rotator_handler->get_settings();

            config::saveUserConfig();
        }

        void loadConfig()
        {
            if (config::main_cfg["user"].contains("recorder_tracking"))
            {
                auto enabled_satellites = getValueOrDefault(config::main_cfg["user"]["recorder_tracking"]["enabled_objects"], std::vector<TrackedObject>());
                nlohmann::json rotator_algo_cfg;
                if (config::main_cfg["user"]["recorder_tracking"].contains("rotator_algo"))
                    rotator_algo_cfg = config::main_cfg["user"]["recorder_tracking"]["rotator_algo"];

                auto_scheduler.setTracked(enabled_satellites);
                object_tracker.setRotatorConfig(rotator_algo_cfg);
                auto_scheduler.setAutoTrackCfg(getValueOrDefault<AutoTrackCfg>(config::main_cfg["user"]["recorder_tracking"]["autotrack_cfg"], AutoTrackCfg()));
            }
        }

    public:
        TrackingWidget();
        ~TrackingWidget();

        void render();
        void renderConfig();
    };
}
