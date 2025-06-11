#pragma once

#include "common/dsp/io/baseband_type.h"
#include "common/widgets/pipeline_selector.h"
#include "image/image.h"
#include "nlohmann/json_utils.h"
#include "passes.h"
#include <functional>

namespace satdump
{
    struct AutoTrackCfg
    {
        float autotrack_min_elevation = 0;
        bool stop_sdr_when_idle = false;
        bool multi_mode = false;
        bool use_localtime = false;
    };

    inline void to_json(nlohmann::ordered_json &j, const AutoTrackCfg &v)
    {
        j["autotrack_min_elevation"] = v.autotrack_min_elevation;
        j["stop_sdr_when_idle"] = v.stop_sdr_when_idle;
        j["multi_mode"] = v.multi_mode;
        j["use_localtime"] = v.use_localtime;
    }

    inline void from_json(const nlohmann::ordered_json &j, AutoTrackCfg &v)
    {
        if (j.contains("autotrack_min_elevation"))
            v.autotrack_min_elevation = j["autotrack_min_elevation"];
        if (j.contains("stop_sdr_when_idle"))
            v.stop_sdr_when_idle = j["stop_sdr_when_idle"];
        if (j.contains("multi_mode"))
            v.multi_mode = j["multi_mode"];
        if (j.contains("use_localtime"))
            v.use_localtime = j["use_localtime"];
    }

    struct TrackedObject
    {
        int norad = -1;
        float min_elevation = 0;

        // Config
        struct Downlink
        {
            uint64_t frequency = 100000000;
            bool record = false;
            bool live = false;
            std::shared_ptr<PipelineUISelector> pipeline_selector = std::make_shared<PipelineUISelector>(true);
            dsp::BasebandType baseband_format = dsp::CS_16;
            int baseband_decimation = 1; // VFO ONLY!
        };
        std::vector<Downlink> downlinks = std::vector<Downlink>(1);
    };

    inline void to_json(nlohmann::ordered_json &j, const TrackedObject &v)
    {
        j["norad"] = v.norad;
        if (v.min_elevation > 0)
            j["min_elevation"] = v.min_elevation;
        for (int i = 0; i < (int)v.downlinks.size(); i++)
        {
            j["downlinks"][i]["frequency"] = v.downlinks[i].frequency;
            j["downlinks"][i]["record"] = v.downlinks[i].record;
            j["downlinks"][i]["live"] = v.downlinks[i].live;
            j["downlinks"][i]["pipeline_name"] = v.downlinks[i].pipeline_selector->selected_pipeline.id;
            j["downlinks"][i]["pipeline_params"] = v.downlinks[i].pipeline_selector->getParameters();
            j["downlinks"][i]["baseband_format"] = (std::string)v.downlinks[i].baseband_format;
            j["downlinks"][i]["baseband_decimation"] = v.downlinks[i].baseband_decimation;

#if defined(BUILD_ZIQ)
            if (v.downlinks[i].baseband_format == dsp::ZIQ)
                j["downlinks"][i]["ziq_depth"] = v.downlinks[i].baseband_format.ziq_depth;
#endif

#if defined(BUILD_ZIQ2)
            if (v.downlinks[i].baseband_format == dsp::ZIQ2)
                j["downlinks"][i]["ziq_depth"] = v.downlinks[i].baseband_format.ziq_depth;
#endif

            nlohmann::ordered_json work_params = nlohmann::ordered_json::object();
            for (auto &step : v.downlinks[i].pipeline_selector->selected_pipeline.steps)
            {
                nlohmann::ordered_json step_params = nlohmann::ordered_json::object();

                nlohmann::ordered_json module_diff =
                    perform_json_diff(pipeline::pipelines_json[v.downlinks[i].pipeline_selector->selected_pipeline.id]["work"][step.level][step.module], step.parameters);
                if (!module_diff.is_null())
                    step_params[step.module] = module_diff;

                if (step_params.size() > 0)
                    work_params[step.level] = step_params;
            }
            if (work_params.size() > 0)
                j["downlinks"][i]["work_params"] = work_params;
        }
    }

    inline void from_json(const nlohmann::ordered_json &j, TrackedObject &v)
    {
        v.norad = j["norad"];
        if (j.contains("min_elevation"))
            v.min_elevation = j["min_elevation"];
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
                    if (j["downlinks"][i].contains("baseband_format"))
                        v.downlinks[i].baseband_format = j["downlinks"][i]["baseband_format"].get<std::string>();
                    if (j["downlinks"][i].contains("baseband_decimation"))
                        v.downlinks[i].baseband_decimation = j["downlinks"][i]["baseband_decimation"];
                    if (j["downlinks"][i].contains("work_params"))
                        for (auto &step : v.downlinks[i].pipeline_selector->selected_pipeline.steps)
                            if (j["downlinks"][i]["work_params"].contains(step.level))
                                if (j["downlinks"][i]["work_params"][step.level].contains(step.module))
                                    step.parameters = merge_json_diffs(step.parameters, j["downlinks"][i]["work_params"][step.level][step.module]);

#if defined(BUILD_ZIQ) || defined(BUILD_ZIQ2)
                    if (j["downlinks"][i].contains("ziq_depth"))
                        v.downlinks[i].baseband_format.ziq_depth = j["downlinks"][i]["ziq_depth"];
#endif
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
        int hovered_sat = -1;
        bool sat_was_hovered = false;

        std::map<int, SatellitePass> vfo_mode_norads_vis;

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
        bool getEngaged();

        std::vector<TrackedObject> getTracked();
        void setTracked(std::vector<TrackedObject> tracked);

        AutoTrackCfg getAutoTrackCfg();
        void setAutoTrackCfg(AutoTrackCfg v);

        void renderAutotrackConfig(double curr_time);

        image::Image getScheduleImage(int width, double curr_time);
    };
} // namespace satdump