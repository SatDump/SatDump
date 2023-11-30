#include "module_xfsk_burst_demod.h"
#include "common/dsp/filter/firdes.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>

namespace demod
{
    XFSKBurstDemodModule::XFSKBurstDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : BaseDemodModule(input_file, output_file_hint, parameters)
    {
        name = "xFSK Burst Demodulator";
        show_freq = false;

        constellation.d_hscale = (80.0 / 5.0) / 100.0;
        constellation.d_vscale = 20.0 / 100.0;

        MIN_SPS = 8;
        MAX_SPS = 32.0;

        // Buffers
        sym_buffer = new int8_t[d_buffer_size * 4];
    }

    void XFSKBurstDemodModule::init()
    {
        BaseDemodModule::initb();

        // Quadrature demod
        qua = std::make_shared<dsp::QuadratureDemodBlock>(agc->output_stream, 1.0f);

        // AGC2
        agc2 = std::make_shared<dsp::AGC2Block<float>>(qua->output_stream, 5.0, 0.01, 0.001);

        rec = std::make_shared<dsp::GardnerClockRecovery2Block>(agc2->output_stream, final_samplerate, d_symbolrate, 0.707, 24);
    }

    XFSKBurstDemodModule::~XFSKBurstDemodModule()
    {
        delete[] sym_buffer;
    }

    void XFSKBurstDemodModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = file_source->getFilesize();
        else
            filesize = 0;

        data_out = std::ofstream(d_output_file_hint + ".soft", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".soft");

        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".soft");
        logger->info("Buffer size : " + std::to_string(d_buffer_size));

        time_t lastTime = 0;

        // Start
        BaseDemodModule::start();
        qua->start();
        agc2->start();
        rec->start();

        uint8_t wip_byte = 0;
        int wipbitn = 0;

        int dat_size = 0;
        while (demod_should_run())
        {
            dat_size = rec->output_stream->read();

            if (dat_size <= 0)
            {
                rec->output_stream->flush();
                continue;
            }

            // Into const
            constellation.pushFloatAndGaussian(rec->output_stream->readBuf, dat_size);

            // Estimate SNR
            snr_estimator.update((complex_t *)rec->output_stream->readBuf, dat_size / 2);
            snr = snr_estimator.snr();

            if (snr > peak_snr)
                peak_snr = snr;

            logger->trace("%f", rec->output_stream->readBuf[0]);

            for (int i = 0; i < dat_size; i++)
                sym_buffer[i] = clamp(rec->output_stream->readBuf[i] * 10);

            if (output_data_type == DATA_FILE)
                data_out.write((char *)sym_buffer, dat_size);
            else
                output_fifo->write((uint8_t *)sym_buffer, dat_size);

            rec->output_stream->flush();

            if (input_data_type == DATA_FILE)
                progress = file_source->getPosition();

            // Update module stats
            module_stats["snr"] = snr;
            module_stats["peak_snr"] = peak_snr;

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%, SNR : " + std::to_string(snr) + "dB," + " Peak SNR: " + std::to_string(peak_snr) + "dB");
            }
        }

        logger->info("Demodulation finished");

        if (input_data_type == DATA_FILE)
            stop();
    }

    void XFSKBurstDemodModule::stop()
    {
        // Stop
        BaseDemodModule::stop();
        qua->stop();
        agc2->stop();
        rec->stop();
        rec->output_stream->stopReader();

        if (output_data_type == DATA_FILE)
            data_out.close();
    }

    std::string XFSKBurstDemodModule::getID()
    {
        return "xfsk_burst_demod";
    }

    std::vector<std::string> XFSKBurstDemodModule::getParameters()
    {
        std::vector<std::string> params = {};
        params.insert(params.end(), BaseDemodModule::getParameters().begin(), BaseDemodModule::getParameters().end());
        return params;
    }

    std::shared_ptr<ProcessingModule> XFSKBurstDemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<XFSKBurstDemodModule>(input_file, output_file_hint, parameters);
    }
}