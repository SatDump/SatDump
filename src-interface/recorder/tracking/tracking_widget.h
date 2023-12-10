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

namespace satdump
{
    namespace tracking
    { // Autotrack (Basic for now, satellites only, Horizons to be done...)
        struct SatellitePass
        {
            int norad;
            double aos_time;
            double los_time;
            float max_elevation;
        };

        struct TrackedObject
        {
            int norad = -1;

            // Config
            double frequency = 100e6;
            bool record = false;
            bool live = false;
            // std::string pipeline_name = "";

            // std::shared_ptr<PipelineUISelector> pipeline_selector = std::make_shared<PipelineUISelector>(true);
        };

        inline void to_json(nlohmann::ordered_json &j, const TrackedObject &v)
        {
            j["norad"] = v.norad;
            j["frequency"] = v.frequency;
            j["record"] = v.record;
            j["live"] = v.live;
            // j["pipeline_name"] = pipelines[v.pipeline_selector->pipeline_id].name;
            // j["pipeline_params"] = v.pipeline_selector->getParameters();
        }

        inline void from_json(const nlohmann::ordered_json &j, TrackedObject &v)
        {
            v.norad = j["norad"];
            v.frequency = j["frequency"];
            v.record = j["record"];
            v.live = j["live"];
            // v.pipeline_selector->select_pipeline(j["pipeline_name"].get<std::string>());
            // if (j.contains("pipeline_params"))
            //     v.pipeline_selector->setParameters(j["pipeline_params"]);
        }
    }

    class TrackingWidget
    {
    private: // QTH Config
        double qth_lon = 0;
        double qth_lat = 0;
        double qth_alt = 0;

    public: // Handlers
        std::function<void(tracking::SatellitePass, tracking::TrackedObject)> aos_callback = [](tracking::SatellitePass, tracking::TrackedObject) {};
        std::function<void(tracking::SatellitePass, tracking::TrackedObject)> los_callback = [](tracking::SatellitePass, tracking::TrackedObject) {};

    private:
        ObjectTracker object_tracker;

        std::shared_ptr<rotator::RotatorHandler> rotator_handler;
        int selected_rotator_handler = 0;

        bool config_window_was_asked = false, show_window_config = false;

    public:
        TrackingWidget();
        ~TrackingWidget();

        void render();
        void renderConfig();
    };
}
