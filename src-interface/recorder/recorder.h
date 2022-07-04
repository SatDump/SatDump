#pragma once

#include "../app.h"
#include "products/products.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "common/dsp_sample_source/dsp_sample_source.h"
#include "common/dsp/splitter.h"
#include "common/dsp/fft.h"
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

        std::string error;

        std::shared_ptr<dsp::DSPSampleSource> source_ptr;
        std::shared_ptr<dsp::SplitterBlock> splitter;
        std::shared_ptr<dsp::FFTBlock> fft;
        std::shared_ptr<dsp::FileSinkBlock> file_sink;
        std::shared_ptr<widgets::FFTPlot> fft_plot;
        std::shared_ptr<widgets::WaterfallPlot> waterfall_plot;

        std::vector<dsp::SourceDescriptor> sources;

        int sdr_select_id = 0;
        std::string sdr_select_string;

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
            return out;
        }

        void deserialize_config(nlohmann::json in)
        {
            show_waterfall = in["show_waterfall"].get<bool>();
            waterfall_ratio = in["waterfall_ratio"].get<float>();
            panel_ratio = in["panel_ratio"].get<float>();
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