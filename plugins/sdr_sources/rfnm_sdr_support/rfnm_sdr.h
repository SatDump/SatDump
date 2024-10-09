#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include <librfnm/device.h>
#include <librfnm/rfnm_fw_api.h>
#include <librfnm/rx_stream.h>
#include "logger.h"
#include "common/rimgui.h"
#include "common/widgets/double_list.h"

class RFNMSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;
    rfnm::device *rfnm_dev_obj;
    rfnm::rx_stream *rfnm_stream;

    widgets::DoubleList samplerate_widget;
    widgets::DoubleList bandwidth_widget;

    int min_gain = 0;
    int max_gain = 0;

    int channel = 0;
    int gain = 0;
    bool fmnotch_enabled = false;
    bool bias_enabled = false;

    void set_gains();
    void set_others();

    void open_sdr();

protected:
    size_t buffer_size = -1;
    // rfnm::rx_buf rx_buffers[rfnm::MIN_RX_BUFCNT];

    /// Temporary correction.
    float corr_mag = 0;
    float corr_phase = 0;
    ///

    std::thread work_thread;
    bool thread_should_run = false;
    complex_t v;
    void mainThread()
    {
        int16_t *buf = (int16_t *)volk_malloc(buffer_size * 2 * sizeof(int16_t), volk_get_alignment());
        size_t elems_read;
        uint64_t timestamp_ns;

        while (thread_should_run)
        {
#if 0
            auto r = rfnm_stream->read((void **)&buf, buffer_size, elems_read, timestamp_ns);
            if (r == RFNM_API_OK)
            {
                logger->info("READ %d\n", elems_read);
                volk_16i_s32f_convert_32f((float *)output_stream->writeBuf, (int16_t *)buf, 32768.0f, elems_read * 2);

                // Very basic, but helps a lot
                if (corr_mag != 0 || corr_phase != 0)
                {
                    float mag_p1 = 1.0f + corr_mag;
                    float phase = corr_phase;
                    for (int i = 0; i < elems_read; i++)
                    {
                        v = output_stream->writeBuf[i];
                        output_stream->writeBuf[i].real = v.real * mag_p1;
                        output_stream->writeBuf[i].imag = v.imag + phase * v.real;
                    }
                }

                output_stream->swap(elems_read);
            }
#else
            auto r = rfnm_stream->read((void **)&output_stream->writeBuf, buffer_size, elems_read, timestamp_ns);
            if (r == RFNM_API_OK)
                output_stream->swap(elems_read);
#endif
            else if (r)
            {
                logger->error("RFNM empty buffer!");
                continue;
            }
        }
    }

public:
    RFNMSource(dsp::SourceDescriptor source)
        : DSPSampleSource(source), samplerate_widget("Samplerate"), bandwidth_widget("Bandwidth")
    {
    }

    ~RFNMSource()
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

    static std::string getID() { return "rfnm"; }
    static std::shared_ptr<dsp::DSPSampleSource> getInstance(dsp::SourceDescriptor source) { return std::make_shared<RFNMSource>(source); }
    static std::vector<dsp::SourceDescriptor> getAvailableSources();
};