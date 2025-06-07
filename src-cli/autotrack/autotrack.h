#pragma once

#include <memory>

#include "common/dsp/fft/fft_pan.h"
#include "common/dsp/path/splitter.h"
#include "common/dsp/resamp/smart_resampler.h"
#include "common/dsp_source_sink/dsp_sample_source.h"
#include "common/widgets/fft_plot.h"

#include "common/tracking/obj_tracker/object_tracker.h"
#include "common/tracking/rotator/rotcl_handler.h"
#include "common/tracking/scheduler/scheduler.h"

#include "common/dsp/io/file_sink.h"
#include "pipeline/live_pipeline.h"

#include "common/dsp/io/net_sink.h"

class AutoTrackApp
{
private: // Device and start-of-chain management
    dsp::SourceDescriptor selected_src;

    std::shared_ptr<dsp::DSPSampleSource> source_ptr;
    std::unique_ptr<dsp::SplitterBlock> splitter;

    uint64_t frequency_hz = 100e6;
    uint64_t current_samplerate = 1e6;
    double xconverter_frequency = 0;

    uint64_t get_samplerate()
    {
        // if (current_decimation > 0)
        //     return current_samplerate / current_decimation;
        // else
        return current_samplerate;
    }

    void set_frequency(uint64_t freq_hz)
    {
        double xconv_freq = frequency_hz = freq_hz;
        if (xconv_freq > xconverter_frequency * 1e6)
            xconv_freq -= xconverter_frequency * 1e6;
        else
            xconv_freq = xconverter_frequency * 1e6 - xconv_freq;

        source_ptr->set_frequency(xconv_freq);
        if (fft_plot)
        {
            fft_plot->frequency = freq_hz;
            if (xconverter_frequency == 0)
                fft_plot->actual_sdr_freq = -1;
            else
                fft_plot->actual_sdr_freq = xconv_freq;
        }
    }

    bool is_started = false;
    void start_device();
    void stop_device();

private:
    float fft_min = -150, fft_max = 150;
    int fft_size = 8192, fft_rate = 30;
    std::unique_ptr<dsp::FFTPanBlock> fft;
    std::unique_ptr<widgets::FFTPlot> fft_plot;

private:
    std::mutex live_pipeline_mtx;
    std::unique_ptr<satdump::pipeline::LivePipeline> live_pipeline;
    satdump::pipeline::Pipeline selected_pipeline;
    nlohmann::json pipeline_params;
    std::string pipeline_output_dir;

    bool is_processing = false;
    void start_processing();
    void stop_processing();

private:
    bool is_recording = false;

    std::shared_ptr<dsp::FileSinkBlock> file_sink;

    void start_recording();
    void stop_recording();

private:
    std::shared_ptr<dsp::NetSinkBlock> udp_sink;

private: // VFO Stuff
    struct VFOInfo
    {
        ////
        std::string id;
        std::string name;
        double freq;

        //// Live
        satdump::pipeline::Pipeline selected_pipeline;
        nlohmann::json pipeline_params;
        std::string output_dir;
        std::shared_ptr<ctpl::thread_pool> lpool;
        std::shared_ptr<satdump::pipeline::LivePipeline> live_pipeline;

        //// Recording
        std::shared_ptr<dsp::SmartResamplerBlock<complex_t>> decim_ptr;
        std::shared_ptr<dsp::FileSinkBlock> file_sink;
    };

    std::mutex vfos_mtx;
    std::vector<VFOInfo> vfo_list;

    void add_vfo_live(std::string id, std::string name, double freq, satdump::pipeline::Pipeline vpipeline, nlohmann::json vpipeline_params);
    void add_vfo_reco(std::string id, std::string name, double freq, dsp::BasebandType type, int decimation = -1);
    void del_vfo(std::string id);

private:
    satdump::ObjectTracker object_tracker;
    satdump::AutoTrackScheduler auto_scheduler;

    std::shared_ptr<rotator::RotatorHandler> rotator_handler;

    void setup_schedular_callbacks();

private:
    time_t web_last_fft_access = 0;
    bool web_fft_is_enabled = false;
    void setup_webserver();
    void stop_webserver();

public:
    void web_heartbeat();

private:
    ctpl::thread_pool main_thread_pool = ctpl::thread_pool(8);

    const nlohmann::json d_settings;
    const nlohmann::json d_parameters;
    const std::string d_output_folder;

public:
    AutoTrackApp(nlohmann::json settings, nlohmann::json parameters, std::string output_folder);
    ~AutoTrackApp();
};

int main_autotrack(int argc, char *argv[]);