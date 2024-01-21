#pragma once

#include "common/widgets/pipeline_selector.h"
#include "passes.h"
#include <functional>

namespace satdump
{
    struct AutoTrackCfg
    {
        float autotrack_min_elevation = 0;
        bool stop_sdr_when_idle = false;
        bool vfo_mode = false;
    };

    inline void to_json(nlohmann::ordered_json &j, const AutoTrackCfg &v)
    {
        j["autotrack_min_elevation"] = v.autotrack_min_elevation;
        j["stop_sdr_when_idle"] = v.stop_sdr_when_idle;
        j["vfo_mode"] = v.vfo_mode;
    }

    inline void from_json(const nlohmann::ordered_json &j, AutoTrackCfg &v)
    {
        if (j.contains("autotrack_min_elevation"))
            v.autotrack_min_elevation = j["autotrack_min_elevation"];
        if (j.contains("stop_sdr_when_idle"))
            v.stop_sdr_when_idle = j["stop_sdr_when_idle"];
        if (j.contains("vfo_mode"))
            v.vfo_mode = j["vfo_mode"];
    }

    struct TrackedObject
    {
        int norad = -1;

        // Config
        struct Downlink
        {
            uint64_t frequency = 100000000;
            bool record = false;
            bool live = false;
            std::shared_ptr<PipelineUISelector> pipeline_selector = std::make_shared<PipelineUISelector>(true);
        };
        std::vector<Downlink> downlinks = std::vector<Downlink>(1);
    };

    inline void to_json(nlohmann::ordered_json &j, const TrackedObject &v)
    {
        j["norad"] = v.norad;
        for (int i = 0; i < (int)v.downlinks.size(); i++)
        {
            j["downlinks"][i]["frequency"] = v.downlinks[i].frequency;
            j["downlinks"][i]["record"] = v.downlinks[i].record;
            j["downlinks"][i]["live"] = v.downlinks[i].live;
            j["downlinks"][i]["pipeline_name"] = pipelines[v.downlinks[i].pipeline_selector->pipeline_id].name;
            j["downlinks"][i]["pipeline_params"] = v.downlinks[i].pipeline_selector->getParameters();
        }
    }

    inline void from_json(const nlohmann::ordered_json &j, TrackedObject &v)
    {
        v.norad = j["norad"];
        if (j.contains("frequency"))
        {
            v.downlinks[0].frequency = j["frequency"];
            if (j.contains("record"))
                v.downlinks[0].record = j["record"];
            if (j.contains("live"))
                v.downlinks[0].live = j["live"];
            if (j.contains("pipeline_name"))
                v.downlinks[0].pipeline_selector->select_pipeline(j["pipeline_name"].get<std::string>());
            if (j.contains("pipeline_params"))
                v.downlinks[0].pipeline_selector->setParameters(j["pipeline_params"]);
        }
        else if (j.contains("downlinks"))
        {
            v.downlinks.resize(j["downlinks"].size());
            for (int i = 0; i < (int)j["downlinks"].size(); i++)
            {
                if (j["downlinks"][i].contains("frequency"))
                {
                    v.downlinks[i].frequency = j["downlinks"][i]["frequency"];
                    if (j["downlinks"][i].contains("record"))
                        v.downlinks[i].record = j["downlinks"][i]["record"];
                    if (j["downlinks"][i].contains("live"))
                        v.downlinks[i].live = j["downlinks"][i]["live"];
                    if (j["downlinks"][i].contains("pipeline_name"))
                        v.downlinks[i].pipeline_selector->select_pipeline(j["downlinks"][i]["pipeline_name"].get<std::string>());
                    if (j["downlinks"][i].contains("pipeline_params"))
                        v.downlinks[i].pipeline_selector->setParameters(j["downlinks"][i]["pipeline_params"]);
                }
            }
        }
    }

    class AutoTrackScheduler
    {
    private: // QTH Config
        double qth_lon = 0;
        double qth_lat = 0;
        double qth_alt = 0;

    private:
        AutoTrackCfg autotrack_cfg;

    private:
        bool has_tle = false;
        std::vector<std::string> satoptions;

    private:
        bool backend_should_run = false;
        std::thread backend_thread;
        void backend_run();

        std::string availablesatssearch, selectedsatssearch;

        std::vector<int> vfo_mode_norads_vis;

    public: // Handlers
        std::function<void(AutoTrackCfg, SatellitePass, TrackedObject)> eng_callback = [](AutoTrackCfg, SatellitePass, TrackedObject) {};
        std::function<void(AutoTrackCfg, SatellitePass, TrackedObject)> aos_callback = [](AutoTrackCfg, SatellitePass, TrackedObject) {};
        std::function<void(AutoTrackCfg, SatellitePass, TrackedObject)> los_callback = [](AutoTrackCfg, SatellitePass, TrackedObject) {};

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

    public:
        AutoTrackScheduler();
        ~AutoTrackScheduler();

        void start();
        void setQTH(double qth_lon, double qth_lat, double qth_alt);
        void setEngaged(bool v, double curr_time);

        std::vector<TrackedObject> getTracked();
        void setTracked(std::vector<TrackedObject> tracked);

        AutoTrackCfg getAutoTrackCfg();
        void setAutoTrackCfg(AutoTrackCfg v);

        void renderAutotrackConfig(bool light_theme, double curr_time);
    };
}