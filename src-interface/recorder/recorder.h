#pragma once

#include "../app.h"
#include "products/products.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "common/dsp/splitter.h"
#include "common/dsp/fft_pan.h"
#include "common/dsp/file_sink.h"
#include "common/widgets/fft_plot.h"
#include "common/widgets/waterfall_plot.h"

#include "pipeline_selector.h"
#include "core/live_pipeline.h"

namespace satdump
{
    class RecorderApplication : public Application
    {
    protected:
        void drawUI();

        double frequency_mhz = 100;
        bool show_waterfall = true;
        bool is_started = false, is_recording = false, is_processing = false;

        int fft_size = 8192; // * 4;

        bool dragging_waterfall = false;
        float waterfall_ratio = 0.7;

        bool dragging_panel = false;
        float panel_ratio = 0.2;

        std::string recorder_filename;
        int select_sample_format = 0;

        std::string sdr_error, error;

        std::shared_ptr<dsp::DSPSampleSource> source_ptr;
        std::shared_ptr<dsp::SplitterBlock> splitter;
        std::shared_ptr<dsp::FFTPanBlock> fft;
        std::shared_ptr<dsp::FileSinkBlock> file_sink;
        std::shared_ptr<widgets::FFTPlot> fft_plot;
        std::shared_ptr<widgets::WaterfallPlot> waterfall_plot;

        std::vector<dsp::SourceDescriptor> sources;

        int sdr_select_id = 0;
        std::string sdr_select_string;

        bool processing_modules_floating_windows = false;

        bool automated_live_output_dir = false;
        PipelineUISelector pipeline_selector;
        std::unique_ptr<satdump::LivePipeline> live_pipeline;
        std::string pipeline_output_dir;
        nlohmann::json pipeline_params;

        int pipeline_preset_id = 0;

        //#ifdef BUILD_ZIQ
        int ziq_bit_depth;
        //#endif

        nlohmann::json serialize_config()
        {
            nlohmann::json out;
            out["show_waterfall"] = show_waterfall;
            out["waterfall_ratio"] = waterfall_ratio;
            out["panel_ratio"] = panel_ratio;
            if (fft_plot && waterfall_plot && fft)
            {
                out["fft_min"] = fft_plot->scale_min;
                out["fft_max"] = fft_plot->scale_max;
                out["fft_avg"] = fft->avg_rate;
            }
            return out;
        }

        void deserialize_config(nlohmann::json in)
        {
            show_waterfall = in["show_waterfall"].get<bool>();
            waterfall_ratio = in["waterfall_ratio"].get<float>();
            panel_ratio = in["panel_ratio"].get<float>();
            if (fft_plot && waterfall_plot && fft)
            {
                if (in.contains("fft_min"))
                    fft_plot->scale_min = in["fft_min"];
                if (in.contains("fft_max"))
                    fft_plot->scale_max = in["fft_max"];
                if (in.contains("fft_avg"))
                    fft->avg_rate = in["fft_avg"];
            }
        }

        void try_load_sdr_settings()
        {
            if (config::main_cfg["user"].contains("recorder_sdr_settings"))
            {
                if (config::main_cfg["user"]["recorder_sdr_settings"].contains(sources[sdr_select_id].name))
                {
                    auto cfg = config::main_cfg["user"]["recorder_sdr_settings"][sources[sdr_select_id].name];
                    source_ptr->set_settings(cfg);
                    if (cfg.contains("samplerate"))
                        source_ptr->set_samplerate(cfg["samplerate"]);
                }
            }
        }

    public:
        RecorderApplication();
        ~RecorderApplication();

        void save_settings()
        {
            config::main_cfg["user"]["recorder_state"] = serialize_config();
            config::saveUserConfig();
        }

    public:
        static std::string getID() { return "recorder"; }
        static std::shared_ptr<Application> getInstance() { return std::make_shared<RecorderApplication>(); }
    };
};