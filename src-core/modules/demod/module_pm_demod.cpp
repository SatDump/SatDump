#include "module_pm_demod.h"
#include "common/dsp/filter/firdes.h"
#include "logger.h"
#include "imgui/imgui.h"

namespace demod
{
    PMDemodModule::PMDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : BaseDemodModule(input_file, output_file_hint, parameters)
    {
        // Buffers
        sym_buffer = new int8_t[d_buffer_size * 4];

        // Parse params
        if (parameters.count("resample_after_pll") > 0)
            d_resample_after_pll = parameters["resample_after_pll"].get<bool>();

        if (parameters.count("pll_bw") > 0)
            d_pll_bw = parameters["pll_bw"].get<float>();
        else
            throw std::runtime_error("PLL Bw parameter must be present!");

        if (parameters.count("pll_max_offset") > 0)
            d_pll_max_offset = parameters["pll_max_offset"].get<float>();

        if (parameters.count("rrc_alpha") > 0)
            d_rrc_alpha = parameters["rrc_alpha"].get<float>();
        else
            throw std::runtime_error("RRC Alpha parameter must be present!");

        if (parameters.count("rrc_taps") > 0)
            d_rrc_taps = parameters["rrc_taps"].get<int>();

        if (parameters.count("costas_bw") > 0)
            d_loop_bw = parameters["costas_bw"].get<float>();

        if (parameters.count("clock_gain_omega") > 0)
            d_clock_gain_omega = parameters["clock_gain_omega"].get<float>();

        if (parameters.count("clock_mu") > 0)
            d_clock_mu = parameters["clock_mu"].get<float>();

        if (parameters.count("clock_gain_mu") > 0)
            d_clock_gain_mu = parameters["clock_gain_mu"].get<float>();

        if (parameters.count("clock_omega_relative_limit") > 0)
            d_clock_omega_relative_limit = parameters["clock_omega_relative_limit"].get<float>();

        if (parameters.count("subcarrier_offset") > 0)
            d_subccarier_offset = parameters["subcarrier_offset"].get<uint64_t>();

        name = "PM Demodulator";
        MAX_SPS = 10; // Here we do NOT want to resample unless really necessary

        show_freq = true;
    }

    void PMDemodModule::init()
    {
        BaseDemodModule::init(!d_resample_after_pll);

        // PLL
        pll = std::make_shared<dsp::PLLCarrierTrackingBlock>(agc->output_stream, d_pll_bw, d_pll_max_offset, -d_pll_max_offset);

        // Domain conversion
        pm_psk = std::make_shared<dsp::PMToBPSK>(pll->output_stream,
                                                 d_resample_after_pll ? d_samplerate : final_samplerate,
                                                 d_subccarier_offset == 0 ? d_symbolrate : d_subccarier_offset);

        if (d_resample_after_pll)
            resampler = std::make_shared<dsp::SmartResamplerBlock<complex_t>>(pm_psk->output_stream, final_samplerate, d_samplerate);

        // RRC
        rrc = std::make_shared<dsp::FIRBlock<complex_t>>(d_resample_after_pll ? resampler->output_stream : pm_psk->output_stream, dsp::firdes::root_raised_cosine(1, final_samplerate, d_symbolrate, d_rrc_alpha, d_rrc_taps));

        // Costas
        costas = std::make_shared<dsp::CostasLoopBlock>(rrc->output_stream, d_loop_bw, 2);

        // Clock recovery
        rec = std::make_shared<dsp::MMClockRecoveryBlock<complex_t>>(costas->output_stream, final_sps, d_clock_gain_omega, d_clock_mu, d_clock_gain_mu, d_clock_omega_relative_limit);
    }

    PMDemodModule::~PMDemodModule()
    {
        delete[] sym_buffer;
    }

    void PMDemodModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = file_source->getFilesize();
        else
            filesize = 0;

        if (output_data_type == DATA_FILE)
        {
            data_out = std::ofstream(d_output_file_hint + ".soft", std::ios::binary);
            d_output_files.push_back(d_output_file_hint + ".soft");
        }

        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".soft");
        logger->info("Buffer size : {:d}", d_buffer_size);

        time_t lastTime = 0;

        // Start
        BaseDemodModule::start();
        pll->start();
        pm_psk->start();
        rrc->start();
        costas->start();
        rec->start();

        int dat_size = 0;
        while (input_data_type == DATA_FILE ? !file_source->eof() : input_active.load())
        {
            dat_size = rec->output_stream->read();

            if (dat_size <= 0)
            {
                rec->output_stream->flush();
                continue;
            }

            // Push into constellation
            constellation.pushComplex(rec->output_stream->readBuf, dat_size);

            // Estimate SNR
            snr_estimator.update(rec->output_stream->readBuf, dat_size);
            snr = snr_estimator.snr();

            if (snr > peak_snr)
                peak_snr = snr;

            // Update freq
            display_freq = dsp::rad_to_hz(pll->getFreq(), final_samplerate);

            for (int i = 0; i < dat_size; i++)
            {
                sym_buffer[i] = clamp(rec->output_stream->readBuf[i].real * 100);
            }

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
            module_stats["freq"] = display_freq;

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

    void PMDemodModule::stop()
    {
        // Stop
        BaseDemodModule::stop();

        pll->stop();
        pm_psk->stop();
        rrc->stop();
        costas->stop();
        rec->stop();
        rec->output_stream->stopReader();

        if (output_data_type == DATA_FILE)
            data_out.close();
    }

    std::string PMDemodModule::getID()
    {
        return "pm_demod";
    }

    std::vector<std::string> PMDemodModule::getParameters()
    {
        std::vector<std::string> params = {"rrc_alpha", "rrc_taps", "pll_bw", "pll_max_offset", "costas_bw", "clock_gain_omega", "clock_mu", "clock_gain_mu", "clock_omega_relative_limit"};
        params.insert(params.end(), BaseDemodModule::getParameters().begin(), BaseDemodModule::getParameters().end());
        return params;
    }

    std::shared_ptr<ProcessingModule> PMDemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<PMDemodModule>(input_file, output_file_hint, parameters);
    }
}