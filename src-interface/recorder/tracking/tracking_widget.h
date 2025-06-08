#pragma once

#include "common/geodetic/geodetic_coordinates.h"
#include "core/config.h"
#include "libs/predict/predict.h"
#include "nlohmann/json_utils.h"
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "common/tracking/obj_tracker/object_tracker.h"
#include "common/tracking/sat_finder/sat_finder_ui.h"
#include "common/tracking/scheduler/scheduler.h"
#include "import_export.h"

namespace satdump
{
    class TrackingWidget
    {
    private: // QTH Config
        double qth_lon = 0;
        double qth_lat = 0;
        double qth_alt = 0;

    private: // Config Exporter
        TrackingImportExport config_import_export;

    public: // Handlers
        std::function<void(AutoTrackCfg, SatellitePass, TrackedObject)> aos_callback = [](AutoTrackCfg, SatellitePass, TrackedObject) {};
        std::function<void(AutoTrackCfg, SatellitePass, TrackedObject)> los_callback = [](AutoTrackCfg, SatellitePass, TrackedObject) {};

    private:
        ObjectTracker object_tracker = ObjectTracker(true);
        AutoTrackScheduler auto_scheduler;

        SatFinder sat_finder;

        std::shared_ptr<rotator::RotatorHandler> rotator_handler;
        std::vector<rotator::RotatorHandlerOption> rotator_options;
        std::string rotator_options_str;
        int selected_rotator_handler = 0;

        bool config_window_was_asked = false, show_window_config = false;
        bool satellite_finder_was_asked = false, show_satellite_finder_window = false;

    private:
        void saveConfig()
        {
            satdump_cfg.main_cfg["user"]["recorder_tracking"]["enabled_objects"] = auto_scheduler.getTracked();
            satdump_cfg.main_cfg["user"]["recorder_tracking"]["rotator_algo"] = object_tracker.getRotatorConfig();
            satdump_cfg.main_cfg["user"]["recorder_tracking"]["autotrack_cfg"] = auto_scheduler.getAutoTrackCfg();
            if (rotator_handler)
                satdump_cfg.main_cfg["user"]["recorder_tracking"]["rotator_config"][rotator_handler->get_id()] = rotator_handler->get_settings();

            satdump_cfg.saveUser();
        }

        void loadConfig()
        {
            if (satdump_cfg.main_cfg["user"].contains("recorder_tracking"))
            {
                auto enabled_satellites = getValueOrDefault(satdump_cfg.main_cfg["user"]["recorder_tracking"]["enabled_objects"], std::vector<TrackedObject>());
                nlohmann::json rotator_algo_cfg;
                if (satdump_cfg.main_cfg["user"]["recorder_tracking"].contains("rotator_algo"))
                    rotator_algo_cfg = satdump_cfg.main_cfg["user"]["recorder_tracking"]["rotator_algo"];

                auto_scheduler.setTracked(enabled_satellites);
                object_tracker.setRotatorConfig(rotator_algo_cfg);
                auto_scheduler.setAutoTrackCfg(getValueOrDefault<AutoTrackCfg>(satdump_cfg.main_cfg["user"]["recorder_tracking"]["autotrack_cfg"], AutoTrackCfg()));
            }
        }

    public:
        TrackingWidget();
        ~TrackingWidget();

        void render();
        void renderConfig();
        void renderSatfinder();
    };
} // namespace satdump
