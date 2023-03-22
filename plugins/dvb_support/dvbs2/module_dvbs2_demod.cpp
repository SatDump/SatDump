#include "module_dvbs2_demod.h"
#include "common/dsp/filter/firdes.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "codings/dvb-s2/modcod_to_cfg.h"

namespace dvb
{
    DVBS2DemodModule::DVBS2DemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : BaseDemodModule(input_file, output_file_hint, parameters),
                                                                                                                          constellation_s2(90.0f / 127.0f, 90.0f / 127.0f, demod_constellation_size)
    {
        // Buffers

        // Parse params
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

        if (parameters.count("freq_prop_factor") > 0)
            freq_propagation_factor = parameters["freq_prop_factor"].get<float>();

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

        if (parameters.count("modcod") > 0)
            d_modcod = parameters["modcod"].get<int>();
        else
            throw std::runtime_error("MODCOD parameter must be present!");

        if (parameters.count("shortframes") > 0)
            d_shortframes = parameters["shortframes"].get<bool>();

        if (parameters.count("pilots") > 0)
            d_pilots = parameters["pilots"].get<bool>();

        if (parameters.count("sof_thresold") > 0)
            d_sof_thresold = parameters["sof_thresold"].get<float>();

        if (parameters.count("ldpc_trials") > 0)
            d_max_ldpc_trials = parameters["ldpc_trials"].get<int>();

        if (parameters.count("mt_bch") > 0)
            d_multithread_bch = parameters["mt_bch"].get<bool>();

        // Window Name in the UI
        name = "DVB-S2 Demodulator";

        ring_buffer = std::make_unique<dsp::RingBuffer<int8_t>>();
        ring_buffer2 = std::make_unique<dsp::RingBuffer<uint8_t>>();
    }

    void DVBS2DemodModule::init()
    {
        BaseDemodModule::init();

        float g1 = 0, g2 = 0;

        // Parse modcod number
        auto cfg = dvbs2::get_dvbs2_cfg(d_modcod, d_shortframes, d_pilots);

        frame_slot_count = cfg.frame_slot_count;
        s2_constellation = cfg.constellation;
        s2_constel_obj_type = cfg.constel_obj_type;
        s2_framesize = cfg.framesize;
        s2_coderate = cfg.coderate;
        g1 = cfg.g1;
        g2 = cfg.g2;

        // RRC
        rrc = std::make_shared<dsp::FIRBlock<complex_t>>(agc->output_stream, dsp::firdes::root_raised_cosine(1, final_samplerate, d_symbolrate, d_rrc_alpha, d_rrc_taps));

        // Clock recovery
        rec = std::make_shared<dsp::MMClockRecoveryBlock<complex_t>>(rrc->output_stream, final_sps, d_clock_gain_omega, d_clock_mu, d_clock_gain_mu, d_clock_omega_relative_limit);

        // Freq correction
        freq_sh = std::make_shared<dsp::FreqShiftBlock>(rec->output_stream, 1, 0);

        // PL (SOF) Synchronization
        pl_sync = std::make_shared<dvbs2::S2PLSyncBlock>(freq_sh->output_stream, frame_slot_count, d_pilots);
        pl_sync->thresold = d_sof_thresold;

        // PLL
        s2_pll = std::make_shared<dvbs2::S2PLLBlock>(pl_sync->output_stream, d_loop_bw);
        s2_pll->pilots = d_pilots;
        s2_pll->constellation = std::make_shared<dsp::constellation_t>(s2_constel_obj_type, g1, g2);
        s2_pll->constellation->make_lut(256);
        s2_pll->frame_slot_count = frame_slot_count;
        s2_pll->pls_code = d_modcod << 2 | d_shortframes << 1 | d_pilots;
        s2_pll->update();

        // BB to soft syms
        s2_bb_to_soft = std::make_shared<dvbs2::S2BBToSoft>(s2_pll->output_stream);
        s2_bb_to_soft->pilots = d_pilots;
        s2_bb_to_soft->constellation = std::make_shared<dsp::constellation_t>(s2_constel_obj_type, g1, g2);
        s2_bb_to_soft->constellation->make_lut(256);
        s2_bb_to_soft->frame_slot_count = frame_slot_count;
        s2_bb_to_soft->deinterleaver = std::make_shared<dvbs2::S2Deinterleaver>(s2_constellation, s2_framesize, s2_coderate);

        // Init the rest
        ldpc_decoder = std::make_unique<dvbs2::BBFrameLDPC>(s2_framesize, s2_coderate);
        bch_decoder = std::make_unique<dvbs2::BBFrameBCH>(s2_framesize, s2_coderate);
        descramber = std::make_unique<dvbs2::BBFrameDescrambler>(s2_framesize, s2_coderate);

        // Info
        logger->info("Output bbframe bits : {:d}", bch_decoder->dataSize());
    }

    DVBS2DemodModule::~DVBS2DemodModule()
    {
    }

    void DVBS2DemodModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = file_source->getFilesize();
        else
            filesize = 0;

        if (output_data_type == DATA_FILE)
        {
            data_out = std::ofstream(d_output_file_hint + ".bbframe", std::ios::binary);
            d_output_files.push_back(d_output_file_hint + ".bbframe");
        }

        logger->info("MODCOD : {:d}", d_modcod);
        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".bbframe");
        logger->info("Buffer size : {:d}", d_buffer_size);

        time_t lastTime = 0;

        // Start
        BaseDemodModule::start();
        rrc->start();
        rec->start();
        freq_sh->start();
        pl_sync->start();
        s2_pll->start();
        s2_bb_to_soft->start();

        ring_buffer->init(1000000);
        if (d_multithread_bch)
            ring_buffer2->init(1000000);

        process_s2_th = std::thread(&DVBS2DemodModule::process_s2, this);
        if (d_multithread_bch)
            process_bch_th = std::thread(&DVBS2DemodModule::process_s2_bch, this);

        int dat_size = 0;
        while (input_data_type == DATA_FILE ? !file_source->eof() : input_active.load())
        {
            dat_size = s2_bb_to_soft->output_stream->read();

            if (dat_size <= 0)
            {
                s2_bb_to_soft->output_stream->flush();
                continue;
            }

            // Push into constellation
            constellation_s2.pushComplexPL(&s2_pll->output_stream->readBuf[0], 90);
            constellation_s2.pushComplexSlots(&s2_pll->output_stream->readBuf[90], frame_slot_count * 90);

            // Estimate SNR over slots
            snr_estimator.update(&s2_pll->output_stream->readBuf[90], frame_slot_count * 90);
            snr = snr_estimator.snr();

            if (snr > peak_snr)
                peak_snr = snr;

            // Get freq
            display_freq = dsp::rad_to_hz(current_freq / final_sps, final_samplerate);

            detected_modcod = s2_bb_to_soft->detect_modcod;
            detected_shortframes = s2_bb_to_soft->detect_shortframes;
            detected_pilots = s2_bb_to_soft->detect_pilots;

            ring_buffer->write(s2_bb_to_soft->output_stream->readBuf, d_shortframes ? 16200 : 64800);

            s2_bb_to_soft->output_stream->flush();

            // Propagate frequency to an earlier rotator, slowly
            current_freq -= s2_pll->getFreq() * freq_propagation_factor;
            freq_sh->set_freq_raw(current_freq);
            // logger->info("Freq {:f}, PLFreq {:f}", current_freq, s2_pll->getFreq());

            // Update module stats
            module_stats["snr"] = snr;
            module_stats["peak_snr"] = peak_snr;
            module_stats["freq"] = display_freq;
            module_stats["ldpc_trials"] = ldpc_trials;
            module_stats["bch_corrections"] = bch_corrections;

            if (input_data_type == DATA_FILE)
                progress = file_source->getPosition();
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, SNR : " + std::to_string(snr) + "dB," + " Peak SNR: " + std::to_string(peak_snr) + "dB");
            }
        }

        should_stop = true;

        if (input_data_type == DATA_FILE)
            stop();
    }

    void DVBS2DemodModule::process_s2()
    {
        int8_t *sym_buffer = new int8_t[64800 * 32];
        uint8_t *repacker_buffer = new uint8_t[64800 * 32];

        while (!should_stop)
        {
            int read = ring_buffer->read(sym_buffer, (d_shortframes ? 16200 : 64800) * dvbs2::simd_type::SIZE);

            if (read <= 0)
                continue;

            ldpc_trials = ldpc_decoder->decode(sym_buffer, d_max_ldpc_trials);

            if (ldpc_trials == -1)
                ldpc_trials = d_max_ldpc_trials;

            for (int ff = 0; ff < dvbs2::simd_type::SIZE; ff++)
            {
                int8_t *buf = &sym_buffer[(d_shortframes ? 16200 : 64800) * ff];

                // Repack
                memset(repacker_buffer, 0, ldpc_decoder->dataSize());
                for (int i = 0; i < ldpc_decoder->dataSize(); i++)
                    repacker_buffer[i / 8] = repacker_buffer[i / 8] << 1 | (buf[i] < 0);

                if (!d_multithread_bch)
                {
                    bch_corrections = bch_decoder->decode(repacker_buffer);

                    // if (bch_corrections == -1)
                    //     logger->error("ERROR");

                    descramber->work(repacker_buffer);

                    if (output_data_type == DATA_FILE)
                        data_out.write((char *)repacker_buffer, bch_decoder->dataSize() / 8);
                    else
                        output_fifo->write((uint8_t *)repacker_buffer, bch_decoder->dataSize() / 8);
                }
                else
                {
                    ring_buffer2->write((uint8_t *)repacker_buffer, ldpc_decoder->dataSize() / 8);
                }
            }
        }

        logger->info("Exit FEC Thead!");

        delete[] sym_buffer;
        delete[] repacker_buffer;
    }

    void DVBS2DemodModule::process_s2_bch()
    {
        logger->info("Starting BCH Thead!");

        uint8_t *repacker_buffer = new uint8_t[64800];

        while (!should_stop)
        {
            int bch_fsize = ldpc_decoder->dataSize() / 8;
            int bch_dsize = bch_decoder->dataSize() / 8;
            int read = ring_buffer2->read(repacker_buffer, bch_fsize);

            if (read <= 0)
                continue;

            bch_corrections = bch_decoder->decode(repacker_buffer);

            // if (bch_corrections == -1)
            //     logger->error("ERROR");

            descramber->work(repacker_buffer);

            if (output_data_type == DATA_FILE)
                data_out.write((char *)repacker_buffer, bch_dsize);
            else
                output_fifo->write((uint8_t *)repacker_buffer, bch_dsize);
        }

        logger->info("Exit BCH Thead!");

        delete[] repacker_buffer;
    }

    void DVBS2DemodModule::stop()
    {
        // Stop
        BaseDemodModule::stop();

        rrc->stop();
        rec->stop();
        freq_sh->stop();
        pl_sync->stop();
        s2_pll->stop();
        s2_bb_to_soft->stop();
        s2_bb_to_soft->output_stream->stopReader();
        ring_buffer->stopWriter();
        ring_buffer->stopReader();
        if (d_multithread_bch)
        {
            ring_buffer2->stopWriter();
            ring_buffer2->stopReader();
        }

        if (process_s2_th.joinable())
            process_s2_th.join();
        if (d_multithread_bch)
            if (process_bch_th.joinable())
                process_bch_th.join();

        if (output_data_type == DATA_FILE)
            data_out.close();
    }

    void DVBS2DemodModule::drawUI(bool window)
    {
        ImGui::Begin(name.c_str(), NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        constellation_s2.draw(); // Constellation
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            // Show SNR information
            ImGui::Button("Signal", {200 * ui_scale, 20 * ui_scale});
            ImGui::Text("Freq : ");
            ImGui::SameLine();
            ImGui::TextColored(IMCOLOR_SYNCING, "%.0f Hz", display_freq);
            snr_plot.draw(snr, peak_snr);
            if (!streamingInput)
                if (ImGui::Checkbox("Show FFT", &show_fft))
                    fft_splitter->set_enabled("fft", show_fft);

            // Header
            ImGui::Button("Header", {200 * ui_scale, 20 * ui_scale});
            ImGui::Text("MODCOD : ");
            ImGui::SameLine();
            ImGui::TextColored(IMCOLOR_SYNCED, UITO_C_STR(detected_modcod));
            ImGui::Text("Frames : ");
            ImGui::SameLine();
            ImGui::TextColored(IMCOLOR_SYNCED, detected_shortframes ? "Short" : "Normal");
            ImGui::Text("Pilots : ");
            ImGui::SameLine();
            ImGui::TextColored(detected_pilots ? IMCOLOR_SYNCED : IMCOLOR_NOSYNC, detected_pilots ? "ON" : "OFF");
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {

            // Show FEC information
            ImGui::Button("FEC", {200 * ui_scale, 20 * ui_scale});
            ldpc_viewer.draw(ldpc_trials, 5, 0, "LDPC Trials :");
            bch_viewer.draw(bch_corrections, 10, 0, "BCH Corrections :");
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();

        drawFFT();
    }

    std::string DVBS2DemodModule::getID()
    {
        return "dvbs2_demod";
    }

    std::vector<std::string> DVBS2DemodModule::getParameters()
    {
        std::vector<std::string> params = {"rrc_alpha", "rrc_taps", "pll_bw", "clock_gain_omega", "clock_mu", "clock_gain_mu", "clock_omega_relative_limit"};
        params.insert(params.end(), BaseDemodModule::getParameters().begin(), BaseDemodModule::getParameters().end());
        return params;
    }

    std::shared_ptr<ProcessingModule> DVBS2DemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<DVBS2DemodModule>(input_file, output_file_hint, parameters);
    }
}