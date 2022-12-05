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

        int selected_fft_size = 0;
        std::vector<int> fft_sizes_lut = {8192, 4096, 2048, 1024};
        int fft_size = 8192; // * 4;

        std::vector<colormaps::Map> waterfall_palettes;
        std::string waterfall_palettes_str;
        int selected_waterfall_palette = 0;

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

        uint64_t current_samplerate = 1e6;

        //#ifdef BUILD_ZIQ
        int ziq_bit_depth;
        //#endif

        nlohmann::json serialize_config()
        {
            nlohmann::json out;
            out["show_waterfall"] = show_waterfall;
            out["waterfall_ratio"] = waterfall_ratio;
            out["panel_ratio"] = panel_ratio;
            out["fft_size"] = fft_size;
            out["waterfall_palette"] = waterfall_palettes[selected_waterfall_palette].name;
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
            if (in.contains("fft_size"))
            {
                fft_size = in["fft_size"].get<int>();
                for (int i = 0; i < 4; i++)
                    if (fft_sizes_lut[i] == fft_size)
                        selected_fft_size = i;
            }
            if (in.contains("waterfall_palette"))
            {
                std::string name = in["waterfall_palette"].get<std::string>();
                for (int i = 0; i < (int)waterfall_palettes.size(); i++)
                    if (waterfall_palettes[i].name == name)
                        selected_waterfall_palette = i;
                waterfall_plot->set_palette(waterfall_palettes[selected_waterfall_palette]);
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
                    {
                        try
                        {
                            source_ptr->set_samplerate(cfg["samplerate"]);
                        }
                        catch (std::exception &e)
                        {
                        }
                    }
                    if (cfg.contains("frequency"))
                    {
                        frequency_mhz = cfg["frequency"].get<uint64_t>() / 1e6;
                        source_ptr->set_frequency(frequency_mhz * 1e6);
                    }
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