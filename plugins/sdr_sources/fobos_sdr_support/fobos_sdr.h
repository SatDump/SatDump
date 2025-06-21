#pragma once

#include "common/dsp/resamp/rational_resampler.h"
#include "common/dsp_source_sink/dsp_sample_source.h"

#include <complex.h>
#include <fobos.h>
#include <thread>

#include "common/rimgui.h"
#include "common/widgets/double_list.h"
#include "logger.h"

class FobosSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;
    fobos_dev_t *fobos_dev_ovj;
    // static int _rx_callback(airspy_transfer *t);

    widgets::DoubleList samplerate_widget;

    std::shared_ptr<dsp::stream<complex_t>> output_stream_local;
    std::shared_ptr<dsp::RationalResamplerBlock<complex_t>> decimator;

    int lna_gain = 0;
    int vga_gain = 0;

    bool direct_sampling = false;

    void set_gains();

    void open_sdr();

    std::thread work_thread;
    bool thread_should_run = false;
    void mainThread()
    {
        auto &ostream = decimator ? decimator->input_stream : output_stream;

        while (thread_should_run)
        {
            unsigned int nsamples = 0;
            int err = fobos_rx_read_sync(fobos_dev_ovj, (float *)ostream->writeBuf, &nsamples);

            if (err)
                continue;

            ostream->swap(nsamples);
        }
    }

public:
    FobosSource(dsp::SourceDescriptor source) : DSPSampleSource(source), samplerate_widget("Samplerate") {}

    ~FobosSource()
    {
        stop();
        close();
    }

    void set_settings(nlohmann::json settings);
    nlohmann::json get_settings();

    void open();
    void start();
    void stop();
    void close();

    void set_frequency(uint64_t frequency);

    void drawControlUI();

    void set_samplerate(uint64_t samplerate);
    uint64_t get_samplerate();

    static std::string getID() { return "fobos"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<FobosSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};