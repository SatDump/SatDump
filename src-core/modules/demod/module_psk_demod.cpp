#include "module_psk_demod.h"
#include "common/dsp/filter/firdes.h"
#include "logger.h"
#include "imgui/imgui.h"

namespace demod
{
    PSKDemodModule::PSKDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : BaseDemodModule(input_file, output_file_hint, parameters)
    {
        // Buffers
        sym_buffer = new int8_t[d_buffer_size * 4];

        // Parse params
        if (parameters.count("constellation") > 0)
            constellation_type = parameters["constellation"].get<std::string>();
        else
            throw std::runtime_error("Constellation type parameter must be present!");

        if (parameters.count("rrc_alpha") > 0)
            d_rrc_alpha = parameters["rrc_alpha"].get<float>();
        else
            throw std::runtime_error("RRC Alpha parameter must be present!");

        if (parameters.count("rrc_taps") > 0)
            d_rrc_taps = parameters["rrc_taps"].get<int>();

        if (parameters.count("pll_bw") > 0)
            d_loop_bw = parameters["pll_bw"].get<float>();
        else
            throw std::runtime_error("PLL BW parameter must be present!");

        if (parameters.count("post_costas_dc") > 0)
            d_post_costas_dc_blocking = parameters["post_costas_dc"].get<bool>();

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

        // BPSK Stuff
        is_bpsk = constellation_type == "bpsk";

        // OQPSK Stuff
        is_oqpsk = constellation_type == "oqpsk";
        if (is_oqpsk)
        {
            MIN_SPS = 1.6;
            MAX_SPS = 2.4;
        }

        // Window Name in the UI
        if (constellation_type == "bpsk")
            name = "BPSK Demodulator";
        else if (constellation_type == "qpsk")
            name = "QPSK Demodulator";
        else if (constellation_type == "oqpsk")
            name = "OQPSK Demodulator";
        else if (constellation_type == "8psk")
            name = "8PSK Demodulator";

        // Show freq
        show_freq = true;
    }

    void PSKDemodModule::init()
    {
        BaseDemodModule::init();

        // RRC
        rrc = std::make_shared<dsp::FIRBlock<complex_t>>(agc->output_stream, dsp::firdes::root_raised_cosine(1, final_samplerate, d_symbolrate, d_rrc_alpha, d_rrc_taps));

        // PLL
        if (constellation_type == "bpsk")
            pll = std::make_shared<dsp::CostasLoopBlock>(rrc->output_stream, d_loop_bw, 2);
        else if (constellation_type == "qpsk" || constellation_type == "oqpsk")
            pll = std::make_shared<dsp::CostasLoopBlock>(rrc->output_stream, d_loop_bw, 4);
        else if (constellation_type == "8psk")
            pll = std::make_shared<dsp::CostasLoopBlock>(rrc->output_stream, d_loop_bw, 8);

        if (d_post_costas_dc_blocking)
            post_pll_dc = std::make_shared<dsp::CorrectIQBlock<complex_t>>(pll->output_stream);

        if (is_oqpsk)
            delay = std::make_shared<dsp::DelayOneImagBlock>(d_post_costas_dc_blocking ? post_pll_dc->output_stream : pll->output_stream);

        // Clock recovery
        rec = std::make_shared<dsp::GardnerClockRecoveryBlock<complex_t>>(is_oqpsk ? delay->output_stream : (d_post_costas_dc_blocking ? post_pll_dc->output_stream : pll->output_stream),
                                                                     final_sps, d_clock_gain_omega, d_clock_mu, d_clock_gain_mu, d_clock_omega_relative_limit);
    }

    PSKDemodModule::~PSKDemodModule()
    {
        delete[] sym_buffer;
    }

    void PSKDemodModule::process()
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

        logger->info("Constellation : " + constellation_type);
        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".soft");
        logger->info("Buffer size : {:d}", d_buffer_size);

        time_t lastTime = 0;

        // Start
        BaseDemodModule::start();
        rrc->start();
        pll->start();
        if (d_post_costas_dc_blocking)
            post_pll_dc->start();
        if (is_oqpsk)
            delay->start();
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

            if (is_bpsk) // BPSK Only uses the Q branch... So don't output useless data
            {
                for (int i = 0; i < dat_size; i++)
                {
                    sym_buffer[i] = clamp(rec->output_stream->readBuf[i].real * 50);
                }
            }
            else // Otherwise, for all other consts, we need both
            {
                for (int i = 0; i < dat_size; i++)
                {
                    sym_buffer[i * 2] = clamp(rec->output_stream->readBuf[i].real * 100);
                    sym_buffer[i * 2 + 1] = clamp(rec->output_stream->readBuf[i].imag * 100);
                }
            }

            rec->output_stream->flush();

            if (output_data_type == DATA_FILE)
                data_out.write((char *)sym_buffer, is_bpsk ? dat_size : dat_size * 2);
            else
                output_fifo->write((uint8_t *)sym_buffer, is_bpsk ? dat_size : dat_size * 2);

            // Update module stats
            module_stats["snr"] = snr;
            module_stats["peak_snr"] = peak_snr;
            module_stats["freq"] = display_freq;

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

    void PSKDemodModule::stop()
    {
        // Stop
        BaseDemodModule::stop();

        rrc->stop();
        pll->stop();
        if (d_post_costas_dc_blocking)
            post_pll_dc->stop();
        if (is_oqpsk)
            delay->stop();
        rec->stop();
        rec->output_stream->stopReader();

        if (output_data_type == DATA_FILE)
            data_out.close();
    }

    std::string PSKDemodModule::getID()
    {
        return "psk_demod";
    }

    std::vector<std::string> PSKDemodModule::getParameters()
    {
        std::vector<std::string> params = {"rrc_alpha", "rrc_taps", "pll_bw", "clock_gain_omega", "clock_mu", "clock_gain_mu", "clock_omega_relative_limit"};
        params.insert(params.end(), BaseDemodModule::getParameters().begin(), BaseDemodModule::getParameters().end());
        return params;
    }

    std::shared_ptr<ProcessingModule> PSKDemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<PSKDemodModule>(input_file, output_file_hint, parameters);
    }
}