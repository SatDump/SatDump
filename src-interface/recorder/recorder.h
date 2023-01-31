#pragma once

#include "../app.h"
#include "products/products.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "common/dsp/resamp/smart_resampler.h"
#include "common/dsp/utils/splitter.h"
#include "common/dsp/fft/fft_pan.h"
#include "common/dsp/io/file_sink.h"
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

        int selected_fft_size = 0;
        std::vector<int> fft_sizes_lut = {8192, 4096, 2048, 1024};
        int fft_size = 8192; // * 4;
        int fft_rate = 120;

        std::vector<colormaps::Map> waterfall_palettes;
        std::string waterfall_palettes_str;
        int selected_waterfall_palette = 0;

        bool dragging_waterfall = false;
        float waterfall_ratio = 0.7;

        bool dragging_panel = false;
        float panel_ratio = 0.2;
        unsigned int last_width = 0;

        std::string recorder_filename;
        int select_sample_format = 0;

        std::string sdr_error, error;

        std::shared_ptr<dsp::DSPSampleSource> source_ptr;
        std::shared_ptr<dsp::SmartResamplerBlock<complex_t>> decim_ptr;
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

        uint64_t current_samplerate = 1e6;
        int current_decimation = 1;

        // #ifdef BUILD_ZIQ
        int ziq_bit_depth;
        // #endif

        nlohmann::json serialize_config();
        void deserialize_config(nlohmann::json in);
        void try_load_sdr_settings();

    private:
        void start();
        void stop();

        void start_processing();
        void stop_processing();

        void start_recording();
        void stop_recording();

        uint64_t get_samplerate() { return current_samplerate / current_decimation; }

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