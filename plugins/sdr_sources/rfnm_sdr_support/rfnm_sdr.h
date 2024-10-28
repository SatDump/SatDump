#pragma once

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "librfnm/librfnm/librfnm.h"
#include "logger.h"
#include "common/rimgui.h"
#include "common/widgets/double_list.h"

class RFNMSource : public dsp::DSPSampleSource
{
protected:
    bool is_open = false, is_started = false;
    librfnm *rfnm_dev_obj;

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
    int buffer_size = -1;
    librfnm_rx_buf rx_buffers[LIBRFNM_MIN_RX_BUFCNT];

    /// Temporary correction.
    float corr_mag = 0;
    float corr_phase = 0;
    ///

    std::thread work_thread;
    bool thread_should_run = false;
    complex_t v;
    void mainThread()
    {
        librfnm_rx_buf *rx_buf;

        while (thread_should_run)
        {
            auto r = rfnm_dev_obj->rx_dqbuf(&rx_buf, channel == 1 ? LIBRFNM_CH1 : LIBRFNM_CH0, 1000);
            if (r == rfnm_api_failcode::RFNM_API_DQBUF_NO_DATA)
            {
                logger->error("RFNM empty buffer!");
                continue;
            }
            else if (r)
            {
                continue;
            }

            volk_16i_s32f_convert_32f((float *)output_stream->writeBuf, (int16_t *)rx_buf->buf, 32768.0f, (buffer_size / 4) * 2);

            rfnm_dev_obj->rx_qbuf(rx_buf);

            // Very basic, but helps a lot
            if (corr_mag != 0 || corr_phase != 0)
            {
                float mag_p1 = 1.0f + corr_mag;
                float phase = corr_phase;
                for (int i = 0; i < (buffer_size / 4); i++)
                {
                    v = output_stream->writeBuf[i];
                    output_stream->writeBuf[i].real = v.real * mag_p1;
                    output_stream->writeBuf[i].imag = v.imag + phase * v.real;
                }
            }

            output_stream->swap(buffer_size / 4);
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