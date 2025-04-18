#include "module_sdpsk_demod.h"
#include "common/dsp/filter/firdes.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>

namespace demod
{
    SDPSKDemodModule::SDPSKDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : BaseDemodModule(input_file, output_file_hint, parameters)
    {
        // Buffers
        sym_buffer = new int8_t[d_buffer_size * 4];

        // Parse params
        if (parameters.count("rrc_alpha") > 0)
            d_rrc_alpha = parameters["rrc_alpha"].get<float>();
        else
            throw satdump_exception("RRC Alpha parameter must be present!");

        if (parameters.count("rrc_taps") > 0)
            d_rrc_taps = parameters["rrc_taps"].get<int>();

        if (parameters.count("clock_alpha") > 0)
        {
            float clock_alpha = parameters["clock_alpha"].get<float>();
            d_clock_gain_omega = pow(clock_alpha, 2) / 4.0;
            d_clock_gain_mu = clock_alpha;
        }

        if (parameters.count("clock_gain_omega") > 0)
            d_clock_gain_omega = parameters["clock_gain_omega"].get<float>();

        if (parameters.count("clock_mu") > 0)
            d_clock_mu = parameters["clock_mu"].get<float>();

        if (parameters.count("clock_gain_mu") > 0)
            d_clock_gain_mu = parameters["clock_gain_mu"].get<float>();

        if (parameters.count("clock_omega_relative_limit") > 0)
            d_clock_omega_relative_limit = parameters["clock_omega_relative_limit"].get<float>();

        name = "SDPSK Demodulator";
        show_freq = false;

        constellation.d_hscale = 2.0; // 80.0 / 100.0;
        constellation.d_vscale = 0.2; // 20.0 / 100.0;

        MIN_SPS = 1.0;
        MAX_SPS = 10.0;
    }

    void SDPSKDemodModule::init()
    {
        BaseDemodModule::initb();

        // Quadrature demod
        qua = std::make_shared<dsp::QuadratureDemodBlock>(agc->output_stream, 1.0f);

        // DC Blocker
        dcb2 = std::make_shared<dsp::CorrectIQBlock<float>>(qua->output_stream);

        // LPF
        rrc = std::make_shared<dsp::FIRBlock<float>>(dcb2->output_stream, dsp::firdes::root_raised_cosine(1, final_samplerate, d_symbolrate, d_rrc_alpha, d_rrc_taps));

        // Clock recovery
        rec = std::make_shared<dsp::MMClockRecoveryBlock<float>>(rrc->output_stream,
                                                                 final_sps, d_clock_gain_omega, d_clock_mu, d_clock_gain_mu, d_clock_omega_relative_limit);
    }

    SDPSKDemodModule::~SDPSKDemodModule()
    {
        delete[] sym_buffer;
    }

    void SDPSKDemodModule::process()
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
        dcb2->start();
        rrc->start();
        rec->start();

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
            constellation.pushFloatAndGaussian(rec->output_stream->readBuf, rec->output_stream->getDataSize());

            // Estimate SNR
            snr_estimator.update((complex_t *)rec->output_stream->readBuf, dat_size / 2);
            snr = snr_estimator.snr();
            signal = snr_estimator.signal();
            noise = snr_estimator.noise();

            if (snr > peak_snr)
                peak_snr = snr;

            for (int i = 0; i < dat_size; i++)
                sym_buffer[i] = clamp(rec->output_stream->readBuf[i] * 400);

            rec->output_stream->flush();

            if (output_data_type == DATA_FILE)
                data_out.write((char *)sym_buffer, dat_size);
            else
                output_fifo->write((uint8_t *)sym_buffer, dat_size);

            if (input_data_type == DATA_FILE)
                progress = file_source->getPosition();

            // Update module stats
            module_stats["snr"] = snr;
            module_stats["peak_snr"] = peak_snr;
            module_stats["signal"] = signal;
            module_stats["noise"] = noise;

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

    void SDPSKDemodModule::stop()
    {
        // Stop
        BaseDemodModule::stop();
        qua->stop();
        dcb2->stop();
        rrc->stop();
        rec->stop();
        rec->output_stream->stopReader();

        if (output_data_type == DATA_FILE)
            data_out.close();
    }

    std::string SDPSKDemodModule::getID()
    {
        return "sdpsk_demod";
    }

    std::vector<std::string> SDPSKDemodModule::getParameters()
    {
        std::vector<std::string> params = {"rrc_alpha", "rrc_taps", "pll_bw", "clock_gain_omega", "clock_mu", "clock_gain_mu", "clock_omega_relative_limit"};
        params.insert(params.end(), BaseDemodModule::getParameters().begin(), BaseDemodModule::getParameters().end());
        return params;
    }

    std::shared_ptr<ProcessingModule> SDPSKDemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<SDPSKDemodModule>(input_file, output_file_hint, parameters);
    }
}