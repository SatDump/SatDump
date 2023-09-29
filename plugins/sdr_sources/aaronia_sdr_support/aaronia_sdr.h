#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include <aaroniartsaapi.h>
#include "logger.h"
#include "common/rimgui.h"
#include <thread>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

class AaroniaSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;
    AARTSAAPI_Handle aaronia_handle;
    AARTSAAPI_DeviceInfo aaronia_dinfo;
    AARTSAAPI_Device aaronia_device;
    AARTSAAPI_Config config, root;

    int selected_samplerate = 0;
    std::string samplerate_option_str;
    std::vector<uint64_t> available_samplerates;
    uint64_t current_samplerate = 0;

    float d_min_level = -20;

    int d_rx_channel = 0;

    float d_level = -20;
    int d_usb_compression = 0;
    int d_agc_mode = 0;
    bool d_enable_amp = 0;
    bool d_enable_preamp = 0;
    bool d_rescale = 0;

    void set_others();
    void set_gains();

    std::thread work_thread;
    bool thread_should_run = false;
    void mainThread()
    {
        AARTSAAPI_Packet packet;
        AARTSAAPI_Result res;

        while (thread_should_run)
        {
            while ((res = AARTSAAPI_GetPacket(&aaronia_device, 0, 0, &packet)) == AARTSAAPI_EMPTY)
#ifdef _WIN32
                Sleep(1);
#else
                usleep(1000);
#endif

            if (res == AARTSAAPI_OK)
            {
                int cnt = packet.num;

                if (cnt <= 0)
                    continue;

                if (cnt > dsp::STREAM_BUFFER_SIZE)
                {
                    logger->critical("Spectran buffer too big!", cnt);
                    continue;
                }

                if (cnt > dsp::STREAM_BUFFER_SIZE)
                {
                    logger->critical("Spectran buffer too big!", cnt);
                    continue;
                }

                // Optionally re-scale to be in the more "standard" 1.0f range
                if (d_rescale)
                    volk_32fc_s32fc_multiply_32fc((lv_32fc_t *)output_stream->writeBuf, (lv_32fc_t *)packet.fp32, 1000, cnt);
                else
                    memcpy(output_stream->writeBuf, (complex_t *)packet.fp32, cnt * sizeof(complex_t));

                output_stream->swap(cnt);

                AARTSAAPI_ConsumePackets(&aaronia_device, 0, 1);
            }
        }
    }

public:
    AaroniaSource(dsp::SourceDescriptor source) : DSPSampleSource(source) {}

    ~AaroniaSource()
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

    static std::string getID() { return "aaronia"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<AaroniaSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};