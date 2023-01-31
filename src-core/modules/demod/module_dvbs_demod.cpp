#include "module_dvbs_demod.h"
#include "common/dsp/filter/firdes.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "dvbs/dvbs_interleaving.h"
#include "dvbs/dvbs_reedsolomon.h"
#include "dvbs/dvbs_scrambling.h"
#include "dvbs/dvbs_defines.h"

namespace demod
{
    DVBSDemodModule::DVBSDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : BaseDemodModule(input_file, output_file_hint, parameters),
          viterbi(0.15, 20, VIT_BUF_SIZE, {PHASE_0, PHASE_90})
    {
        if (parameters.count("rrc_alpha") > 0)
            d_rrc_alpha = parameters["rrc_alpha"].get<float>();

        if (parameters.count("rrc_taps") > 0)
            d_rrc_taps = parameters["rrc_taps"].get<int>();

        if (parameters.count("pll_bw") > 0)
            d_loop_bw = parameters["pll_bw"].get<float>();
        else
            throw std::runtime_error("PLL BW parameter must be present!");

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

        name = "DVB-S Demodulator (WIP)";

        // Show freq
        show_freq = true;
    }

    void DVBSDemodModule::init()
    {
        BaseDemodModule::init();

        // RRC
        rrc = std::make_shared<dsp::FIRBlock<complex_t>>(agc->output_stream, dsp::firdes::root_raised_cosine(1, final_samplerate, d_symbolrate, d_rrc_alpha, d_rrc_taps));

        // PLL
        pll = std::make_shared<dsp::CostasLoopBlock>(rrc->output_stream, d_loop_bw, 4);

        // Clock recovery
        rec = std::make_shared<dsp::MMClockRecoveryBlock<complex_t>>(pll->output_stream, final_sps, d_clock_gain_omega, d_clock_mu, d_clock_gain_mu, d_clock_omega_relative_limit);

        // Samples to soft
        sts = std::make_shared<dvbs::DVBSymToSoftBlock>(rec->output_stream, d_buffer_size);
        sts->syms_callback = [this](complex_t *buf, int size)
        {
            // Push into constellation
            constellation.pushComplex(buf, size);

            // Estimate SNR
            snr_estimator.update(buf, size);
            snr = snr_estimator.snr();

            if (snr > peak_snr)
                peak_snr = snr;

            // Update freq
            display_freq = dsp::rad_to_hz(pll->getFreq(), final_samplerate);
        };

        // Viterbi
        vit = std::make_shared<dvbs::DVBSVitBlock>(sts->output_stream);
        vit->viterbi = &viterbi;

        // Deframer
        def = std::make_shared<dvbs::DVBSDefra>(vit->output_stream);
        def->ts_deframer = &ts_deframer;
    }

    DVBSDemodModule::~DVBSDemodModule()
    {
    }

    void DVBSDemodModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = file_source->getFilesize();
        else
            filesize = 0;

        if (output_data_type == DATA_FILE)
        {
            data_out = std::ofstream(d_output_file_hint + ".ts", std::ios::binary);
            d_output_files.push_back(d_output_file_hint + ".ts");
        }

        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".ts");
        logger->info("Buffer size : {:d}", d_buffer_size);

        time_t lastTime = 0;

        // Start
        BaseDemodModule::start();
        rrc->start();
        pll->start();
        rec->start();
        sts->start();
        vit->start();
        def->start();

        uint8_t deinterleaved_frame[204 * 8];

        dvbs::DVBSInterleaving dvb_interleaving;
        dvbs::DVBSReedSolomon reed_solomon;
        dvbs::DVBSScrambling scrambler;

        int dat_size = 0;
        while (input_data_type == DATA_FILE ? !file_source->eof() : input_active.load())
        {
            // Handle outputs
            // Get rate
            std::string rate = "";
            if (viterbi.rate() == viterbi::RATE_1_2)
                rate = "1/2";
            else if (viterbi.rate() == viterbi::RATE_2_3)
                rate = "2/3";
            else if (viterbi.rate() == viterbi::RATE_3_4)
                rate = "3/4";
            else if (viterbi.rate() == viterbi::RATE_5_6)
                rate = "5/6";
            else if (viterbi.rate() == viterbi::RATE_7_8)
                rate = "7/8";

            // Update module stats
            module_stats["snr"] = snr;
            module_stats["peak_snr"] = peak_snr;
            module_stats["freq"] = display_freq;
            module_stats["viterbi_ber"] = viterbi.ber();
            module_stats["viterbi_lock"] = viterbi.getState();
            module_stats["viterbi_rate"] = rate;
            module_stats["rs_avg"] = (errors[0] + errors[1] + errors[2] + errors[3] + errors[4] + errors[5] + errors[6] + errors[7]) / 8;

            if (input_data_type == DATA_FILE)
                progress = file_source->getPosition();
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);

                std::string viterbi_l = std::string(viterbi.getState() == 0 ? "NOSYNC" : "SYNC") + " " + rate;
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, SNR : " + std::to_string(snr) + "dB, Viterbi : " + viterbi_l + ", Peak SNR: " + std::to_string(peak_snr) + "dB");
            }

            // Read data
            dat_size = def->output_stream->read();

            if (dat_size <= 0)
            {
                def->output_stream->flush();
                continue;
            }

#if 1
            int frm_cnt = dat_size / (204 * 8);

            for (int i = 0; i < frm_cnt; i++)
            {
                uint8_t *current_frame = &def->output_stream->readBuf[i * 204 * 8];

                dvb_interleaving.deinterleave(current_frame, deinterleaved_frame);

                for (int i = 0; i < 8; i++)
                    errors[i] = reed_solomon.decode(&deinterleaved_frame[204 * i]);

                scrambler.descramble(deinterleaved_frame);

                for (int i = 0; i < 8; i++)
                {
                    if (output_data_type == DATA_FILE)
                        data_out.write((char *)&deinterleaved_frame[204 * i], 188);
                    else
                        output_fifo->write((uint8_t *)&deinterleaved_frame[204 * i], 188);
                }
            }

#else
            if (output_data_type == DATA_FILE)
                data_out.write((char *)def->output_stream->readBuf, dat_size);
            else
                output_fifo->write((uint8_t *)def->output_stream->readBuf, dat_size);
#endif
            def->output_stream->flush();
        }

        logger->info("Demodulation finished");

        if (input_data_type == DATA_FILE)
            stop();
    }

    void DVBSDemodModule::stop()
    {
        // Stop
        BaseDemodModule::stop();

        rrc->stop();
        pll->stop();
        rec->stop();
        sts->stop();
        vit->stop();
        def->stop();
        def->output_stream->stopReader();

        if (output_data_type == DATA_FILE)
            data_out.close();
    }

    void DVBSDemodModule::drawUI(bool window)
    {
        ImGui::Begin(name.c_str(), NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        constellation.draw(); // Constellation
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            // Show SNR information
            ImGui::Button("Signal", {200 * ui_scale, 20 * ui_scale});
            if (show_freq)
            {
                ImGui::Text("Freq : ");
                ImGui::SameLine();
                ImGui::TextColored(IMCOLOR_SYNCING, "%.0f Hz", display_freq);
            }
            snr_plot.draw(snr, peak_snr);
            if (!streamingInput)
                if (ImGui::Checkbox("Show FFT", &show_fft))
                    fft_splitter->set_output_2nd(show_fft);

            ImGui::Spacing();

            ImGui::Button("Viterbi", {200 * ui_scale, 20 * ui_scale});
            {
                float ber = viterbi.ber();

                ImGui::Text("State : ");

                ImGui::SameLine();

                std::string rate = "";
                if (viterbi.rate() == viterbi::RATE_1_2)
                    rate = "1/2";
                else if (viterbi.rate() == viterbi::RATE_2_3)
                    rate = "2/3";
                else if (viterbi.rate() == viterbi::RATE_3_4)
                    rate = "3/4";
                else if (viterbi.rate() == viterbi::RATE_5_6)
                    rate = "5/6";
                else if (viterbi.rate() == viterbi::RATE_7_8)
                    rate = "7/8";

                if (viterbi.getState() == 0)
                    ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                else
                    ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED %s", rate.c_str());

                ImGui::Text("BER   : ");
                ImGui::SameLine();
                ImGui::TextColored(viterbi.getState() == 0 ? IMCOLOR_NOSYNC : IMCOLOR_SYNCED, UITO_C_STR(ber));

                std::memmove(&ber_history[0], &ber_history[1], (200 - 1) * sizeof(float));
                ber_history[200 - 1] = ber;

                ImGui::PlotLines("", ber_history, IM_ARRAYSIZE(ber_history), 0, "", 0.0f, 1.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
            }

            ImGui::Spacing();

            ImGui::Button("Reed-Solomon", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("RS    : ");
                for (int i = 0; i < 8; i++)
                {
                    ImGui::SameLine();

                    if (errors[i] == -1)
                        ImGui::TextColored(IMCOLOR_NOSYNC, "%i ", i);
                    else if (errors[i] > 0)
                        ImGui::TextColored(IMCOLOR_SYNCING, "%i ", i);
                    else
                        ImGui::TextColored(IMCOLOR_SYNCED, "%i ", i);
                }
            }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();

        drawFFT();
    }

    std::string DVBSDemodModule::getID()
    {
        return "dvbs_demod";
    }

    std::vector<std::string> DVBSDemodModule::getParameters()
    {
        std::vector<std::string> params = {"rrc_alpha", "rrc_taps", "pll_bw", "clock_gain_omega", "clock_mu", "clock_gain_mu", "clock_omega_relative_limit"};
        params.insert(params.end(), BaseDemodModule::getParameters().begin(), BaseDemodModule::getParameters().end());
        return params;
    }

    std::shared_ptr<ProcessingModule> DVBSDemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<DVBSDemodModule>(input_file, output_file_hint, parameters);
    }
}