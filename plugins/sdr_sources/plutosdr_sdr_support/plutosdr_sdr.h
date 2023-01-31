#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#ifdef __ANDROID__
#include "iio.h"
#include "ad9361.h"
#else
#include <iio.h>
#include <ad9361.h>
#endif
#include "logger.h"
#include "imgui/imgui.h"
#include "core/style.h"
#include <thread>

class PlutoSDRSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;

    iio_context *ctx = NULL;
    iio_device *phy = NULL;
    iio_device *dev = NULL;

    int selected_samplerate = 0;
    std::string samplerate_option_str;
    std::vector<uint64_t> available_samplerates;
    uint64_t current_samplerate = 0;

    bool is_usb = false;

    int gain = 0;
    int gain_mode = 0;
    std::string ip_address = "192.168.2.1";

    void set_gains();

    std::thread work_thread;
    bool thread_should_run = false;
    std::mutex work_thread_mtx;

    bool auto_reconnect = false;

    void mainThread()
    {
        work_thread_mtx.lock();

    restart:
        int blockSize = std::min<int>(current_samplerate / 250, dsp::STREAM_BUFFER_SIZE);

        struct iio_channel *rx0_i, *rx0_q;
        struct iio_buffer *rxbuf;

        rx0_i = iio_device_find_channel(dev, "voltage0", 0);
        rx0_q = iio_device_find_channel(dev, "voltage1", 0);

        iio_channel_enable(rx0_i);
        iio_channel_enable(rx0_q);

        rxbuf = iio_device_create_buffer(dev, blockSize, false);
        if (!rxbuf)
        {
            logger->error("Could not create RX buffer");
            return;
        }

        while (thread_should_run)
        {
            if (iio_buffer_refill(rxbuf) < 0)
                break;
            int16_t *buf = (int16_t *)iio_buffer_first(rxbuf, rx0_i);
            volk_16i_s32f_convert_32f((float *)output_stream->writeBuf, buf, 32768.0f, blockSize * 2);
            output_stream->swap(blockSize);
        }

        iio_buffer_destroy(rxbuf);

        if (thread_should_run && auto_reconnect && !is_usb)
        {
            iio_context_destroy(ctx);
            is_started = false;
            while (thread_should_run)
            {
                logger->trace("Trying to reconnect PlutoSDR");
                try
                {
                    sdr_startup();
                    goto restart;
                }
                catch (std::runtime_error &e)
                {
                    logger->trace(e.what());
                }
            }
        }

        work_thread_mtx.unlock();
    }

    void start_thread()
    {
        work_thread_mtx.lock();
        work_thread_mtx.unlock();

        if (!thread_should_run)
        {
            thread_should_run = true;
            work_thread = std::thread(&PlutoSDRSource::mainThread, this);
        }
    }

    void stop_thread()
    {
        thread_should_run = false;
        logger->info("Waiting for the thread...");
        if (is_started)
            output_stream->stopWriter();
        if (work_thread.joinable())
            work_thread.join();
        logger->info("Thread stopped");
    }

    void sdr_startup();

public:
    PlutoSDRSource(dsp::SourceDescriptor source) : DSPSampleSource(source)
    {
        thread_should_run = false;
    }

    ~PlutoSDRSource()
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

    static std::string getID() { return "plutosdr"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<PlutoSDRSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};
