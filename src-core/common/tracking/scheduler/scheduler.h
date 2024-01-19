#pragma once

#include "common/widgets/pipeline_selector.h"
#include "passes.h"
#include <functional>

namespace satdump
{
    struct AutoTrackCfg
    {
        bool stop_sdr_when_idle = false;
    };

    inline void to_json(nlohmann::ordered_json &j, const AutoTrackCfg &v)
    {
        j["stop_sdr_when_idle"] = v.stop_sdr_when_idle;
    }

    inline void from_json(const nlohmann::ordered_json &j, AutoTrackCfg &v)
    {
        v.stop_sdr_when_idle = j["stop_sdr_when_idle"];
    }

    struct TrackedObject
    {
        int norad = -1;

        // Config
        uint64_t frequency = 100000000;
        bool record = false;
        bool live = false;
        // std::string pipeline_name = "";

        std::shared_ptr<PipelineUISelector> pipeline_selector = std::make_shared<PipelineUISelector>(true);
    };

    inline void to_json(nlohmann::ordered_json &j, const TrackedObject &v)
    {
        j["norad"] = v.norad;
        j["frequency"] = v.frequency;
        j["record"] = v.record;
        j["live"] = v.live;
        j["pipeline_name"] = pipelines[v.pipeline_selector->pipeline_id].name;
        j["pipeline_params"] = v.pipeline_selector->getParameters();
    }

    inline void from_json(const nlohmann::ordered_json &j, TrackedObject &v)
    {
        v.norad = j["norad"];
        if (j.contains("frequency"))
            v.frequency = j["frequency"];
        if (j.contains("record"))
            v.record = j["record"];
        if (j.contains("live"))
            v.live = j["live"];
        if (j.contains("pipeline_name"))
            v.pipeline_selector->select_pipeline(j["pipeline_name"].get<std::string>());
        if (j.contains("pipeline_params"))
            v.pipeline_selector->setParameters(j["pipeline_params"]);
    }

    class AutoTrackScheduler
    {
    private: // QTH Config
        double qth_lon = 0;
        double qth_lat = 0;
        double qth_alt = 0;

    private:
        bool has_tle = false;
        std::vector<std::string> satoptions;

    private:
        bool backend_should_run = false;
        std::thread backend_thread;
        void backend_run();

        std::string availablesatssearch, selectedsatssearch;

    public: // Handlers
        std::function<void(AutoTrackCfg, SatellitePass, TrackedObject)> eng_callback = [](AutoTrackCfg, SatellitePass, TrackedObject) {};
        std::function<void(AutoTrackCfg, SatellitePass, TrackedObject)> aos_callback = [](AutoTrackCfg, SatellitePass, TrackedObject) {};
        std::function<void(AutoTrackCfg, SatellitePass, TrackedObject)> los_callback = [](AutoTrackCfg, SatellitePass, TrackedObject) {};

        AutoTrackCfg autotrack_cfg;

    private:
        int tracking_sats_menu_selected_1 = 0, tracking_sats_menu_selected_2 = 0;
        std::vector<TrackedObject> enabled_satellites;

        std::mutex upcoming_satellite_passes_mtx;
        std::vector<SatellitePass> upcoming_satellite_passes_all;
        std::vector<SatellitePass> upcoming_satellite_passes_sel;

        bool autotrack_engaged = false;

        void processAutotrack(double curr_time);
        void updateAutotrackPasses(double curr_time);

        bool autotrack_pass_has_started = false;

    private:
        float autotrack_min_elevation = 0;

    public:
        AutoTrackScheduler();
        ~AutoTrackScheduler();

        void start();
        void setQTH(double qth_lon, double qth_lat, double qth_alt);
        void setEngaged(bool v, double curr_time);

        std::vector<TrackedObject> getTracked();
        void setTracked(std::vector<TrackedObject> tracked);

        float getMinElevation();
        void setMinElevation(float v);

        void renderAutotrackConfig(double curr_time);
    };
}