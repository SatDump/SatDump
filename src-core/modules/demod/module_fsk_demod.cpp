#include "module_fsk_demod.h"
#include "common/dsp/firdes.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>

namespace demod
{
    FSKDemodModule::FSKDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : BaseDemodModule(input_file, output_file_hint, parameters)
    {
        // Buffers
        sym_buffer = new int8_t[d_buffer_size * 10];

        // Parse params
        if (parameters.count("lpf_cutoff") > 0)
            d_lpf_cutoff = parameters["lpf_cutoff"].get<int>();
        else
            throw std::runtime_error("LPT Cutoff parameter must be present!");

        if (parameters.count("lpf_transition_width") > 0)
            d_lpf_transition_width = parameters["lpf_transition_width"].get<int>();
        else
            throw std::runtime_error("LPF Transition Width parameter must be present!");

        name = "FSK Demodulator";
        show_freq = false;

        constellation.d_hscale = 50.0 / 100.0;
        constellation.d_vscale = 20.0 / 100.0;
    }

    void FSKDemodModule::init()
    {
        BaseDemodModule::init();

        // LPF
        lpf = std::make_shared<dsp::CCFIRBlock>(agc->output_stream, dsp::firdes::low_pass(1, d_samplerate, d_lpf_cutoff, d_lpf_transition_width, dsp::fft::window::WIN_KAISER));

        // Quadrature demod
        qua = std::make_shared<dsp::QuadratureDemodBlock>(lpf->output_stream, 1.0f);

        // Clock recovery
        rec = std::make_shared<dsp::FFMMClockRecoveryBlock>(qua->output_stream, (float)d_samplerate / (float)d_symbolrate, powf(0.01f, 2) / 4.0f, 0.5f, 0.01, 100e-6f);
    }

    FSKDemodModule::~FSKDemodModule()
    {
        delete[] sym_buffer;
    }

    void FSKDemodModule::process()
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
        lpf->start();
        qua->start();
        rec->start();

        int dat_size = 0;
        while (input_data_type == DATA_FILE ? !file_source->eof() : input_active.load())
        {
            dat_size = rec->output_stream->read();

            if (dat_size <= 0)
                continue;

            // Into const
            constellation.pushFloatAndGaussian(rec->output_stream->readBuf, rec->output_stream->getDataSize());

            // Estimate SNR
            snr_estimator.update((complex_t *)rec->output_stream->readBuf, dat_size / 2);
            snr = snr_estimator.snr();

            if (snr > peak_snr)
                peak_snr = snr;

            for (int i = 0; i < dat_size; i++)
                sym_buffer[i] = clamp(rec->output_stream->readBuf[i] * 50);

            rec->output_stream->flush();

            if (output_data_type == DATA_FILE)
                data_out.write((char *)sym_buffer, dat_size);
            else
                output_fifo->write((uint8_t *)sym_buffer, dat_size);

            if (input_data_type == DATA_FILE)
                progress = file_source->getPosition();
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, SNR : " + std::to_string(snr) + "dB," + " Peak SNR: " + std::to_string(peak_snr) + "dB");
            }
        }

        logger->info("Demodulation finished");

        if (input_data_type == DATA_FILE)
            stop();
    }

    void FSKDemodModule::stop()
    {
        // Stop
        BaseDemodModule::stop();
        lpf->stop();
        qua->stop();
        rec->stop();
        rec->output_stream->stopReader();

        if (output_data_type == DATA_FILE)
            data_out.close();
    }

    std::string FSKDemodModule::getID()
    {
        return "fsk_demod";
    }

    std::vector<std::string> FSKDemodModule::getParameters()
    {
        return {"lpf_cutoff", "lpf_transition_width"};
    }

    std::shared_ptr<ProcessingModule> FSKDemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<FSKDemodModule>(input_file, output_file_hint, parameters);
    }
}