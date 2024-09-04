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
            throw satdump_exception("PLL Bw parameter must be present!");

        if (parameters.count("pll_max_offset") > 0)
            d_pll_max_offset = parameters["pll_max_offset"].get<float>();

        if (parameters.count("rrc_alpha") > 0)
            d_rrc_alpha = parameters["rrc_alpha"].get<float>();
        else
            throw satdump_exception("RRC Alpha parameter must be present!");

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
        BaseDemodModule::initb(!d_resample_after_pll);

        // PLL
        npll.d_loop_bw = d_pll_bw;
        npll.d_max_freq = d_pll_max_offset;
        npll.d_min_freq = -d_pll_max_offset;

        npll.set_input(0, nagc.get_output(0));

        std::shared_ptr<NaFiFo> next_out = npll.get_output(0);

        // Domain conversion
        pm_psk.d_samplerate = d_resample_after_pll ? d_samplerate : final_samplerate;
        pm_psk.d_symbolrate = d_subccarier_offset == 0 ? d_symbolrate : d_subccarier_offset;

        pm_psk.set_input(0, next_out);
        next_out = pm_psk.get_output(0);

        if (d_resample_after_pll)
        {
            nresampler.d_interpolation = final_samplerate;
            nresampler.d_decimation = d_samplerate;

            nresampler.set_input(0, next_out);
            next_out = nresampler.get_output(0);

            // AGC2
            nagc2.d_rate = 0.001;
            nagc2.d_reference = 1.0;
            nagc2.d_gain = 1.0;
            nagc2.d_max_gain = 1000;

            nagc2.set_input(0, next_out);
            next_out = nagc2.get_output(0);
        }

        // RRC
        nrrc.d_taps = dsp::firdes::root_raised_cosine(1, final_samplerate, d_symbolrate, d_rrc_alpha, d_rrc_taps);

        nrrc.set_input(0, next_out);
        next_out = nrrc.get_output(0);

        // Costas
        ncostas.d_loop_bw = d_loop_bw;
        ncostas.d_order = 2;
        ncostas.d_freq_limit = 1;

        ncostas.set_input(0, next_out);
        next_out = ncostas.get_output(0);

        // Clock recovery
        nrec.d_omega = final_sps;
        nrec.d_omega_gain = d_clock_gain_omega;
        nrec.d_mu = d_clock_mu;
        nrec.d_mu_gain = d_clock_gain_mu;
        nrec.d_omega_relative_limit = d_clock_omega_relative_limit;

        nrec.set_input(0, next_out);
    }

    PMDemodModule::~PMDemodModule()
    {
        delete[] sym_buffer;
    }

    void PMDemodModule::process()
    {
        if (output_data_type == DATA_FILE)
        {
            data_out = std::ofstream(d_output_file_hint + ".soft", std::ios::binary);
            d_output_files.push_back(d_output_file_hint + ".soft");
        }

        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".soft");
        logger->info("Buffer size : %d", d_buffer_size);

        time_t lastTime = 0;

        // Start
        BaseDemodModule::start();

        // TODO FIGURE OUT
        if (input_data_type == DATA_FILE)
            filesize = nfile_source.filesize();
        else
            filesize = 0;

        npll.start();
        pm_psk.start();
        if (d_resample_after_pll)
            nagc2.start();
        nrrc.start();
        ncostas.start();
        nrec.start();

        int dat_size = 0;
        while (demod_should_run())
        {
            if (!nrec.outputs[0]->read())
            {
                auto *rbuf = (ndsp::buf::StdBuf<complex_t> *)nrec.outputs[0]->read_buf();
                dat_size = rbuf->cnt;

                if (dat_size <= 0)
                {
                    nrec.outputs[0]->flush();
                    continue;
                }

                // Push into constellation
                constellation.pushComplex(rbuf->dat, dat_size);

                // Estimate SNR
                snr_estimator.update(rbuf->dat, dat_size);
                snr = snr_estimator.snr();

                if (snr > peak_snr)
                    peak_snr = snr;

                // Update freq
                display_freq = dsp::rad_to_hz(npll.getFreq(), final_samplerate);

                for (int i = 0; i < dat_size; i++)
                {
                    sym_buffer[i] = clamp(rbuf->dat[i].real * 100);
                }

                nrec.outputs[0]->flush();

                if (output_data_type == DATA_FILE)
                    data_out.write((char *)sym_buffer, dat_size);
                else
                    output_fifo->write((uint8_t *)sym_buffer, dat_size);

                if (input_data_type == DATA_FILE)
                    progress = nfile_source.position();

                // Update module stats
                module_stats["snr"] = snr;
                module_stats["peak_snr"] = peak_snr;
                module_stats["freq"] = display_freq;

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%, SNR : " + std::to_string(snr) + "dB," + " Peak SNR: " + std::to_string(peak_snr) + "dB");
                }
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

        npll.stop();
        pm_psk.stop();
        if (d_resample_after_pll)
            nagc2.stop();
        nrrc.stop();
        ncostas.stop();
        nrec.stop();

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