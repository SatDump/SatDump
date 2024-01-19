#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "common/widgets/timed_message.h"
#include "spyserver/spyserver_client.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"

class SpyServerSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_connected = false, is_started = false;
    spyserver::SpyServerClient client;

    int selected_samplerate = 0;
    std::string samplerate_option_str;
    std::vector<uint64_t> available_samplerates;
    uint64_t current_samplerate = 0;

    int selected_bit_depth = 0;

    std::string ip_address = "0.0.0.0";
    int port = 5555;
    int bit_depth = 32;
    int gain = 10;
    int digital_gain = 0;

    int stage_to_use = 0;

    void set_gains();
    void set_bias();
    void set_agcs();

    widgets::TimedMessage error;

    uint64_t buffer_samplerate = 0;

    void try_connect()
    {
        output_stream = std::make_shared<dsp::stream<complex_t>>();
        client = spyserver::connect(ip_address, port, output_stream.get());
        if (client == NULL)
            throw std::runtime_error("Connection error!");

        if (!client->waitForDevInfo(4000))
            throw std::runtime_error("Didn't get dev info!");

        // Get available samplerates
        available_samplerates.clear();
        for (int dec_stage = client->devInfo.MinimumIQDecimation; dec_stage < (int)client->devInfo.DecimationStageCount; dec_stage++)
        {
            uint64_t rate = client->devInfo.MaximumSampleRate / (1 << dec_stage);
            logger->trace("SpyServer has samplerate %d SPS", rate);
            available_samplerates.push_back(rate);
        }

        // Init UI stuff
        samplerate_option_str = "";
        for (uint64_t samplerate : available_samplerates)
            samplerate_option_str += format_notated(samplerate, "sps") + '\0';

        is_connected = true;

        if (buffer_samplerate != 0)
            set_samplerate(buffer_samplerate);
    }

    void disconnect()
    {
        if (is_connected)
            client->close();
        is_connected = false;
    }

public:
    SpyServerSource(dsp::SourceDescriptor source) : DSPSampleSource(source) {}

    ~SpyServerSource()
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

    static std::string getID() { return "spyserver"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<SpyServerSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};