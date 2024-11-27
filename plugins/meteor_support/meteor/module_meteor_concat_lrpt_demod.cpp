#include "module_meteor_concat_lrpt_demod.h"
#include "common/widgets/themed_widgets.h"
#include "common/codings/correlator.h"
#include "common/codings/randomization.h"
#include "common/dsp/filter/firdes.h"
#include "common/codings/differential/nrzm.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "deint.h"
#include "dint_sample_reader.h"

#define BUFFER_SIZE 8192
#define FRAME_SIZE 1024
#define ENCODED_FRAME_SIZE 1024 * 8 * 2

// TODO: Enforce RS check if requested

namespace meteor
{
    MeteorConcatLrptDemod::MeteorConcatLrptDemod(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : BaseDemodModule(input_file, output_file_hint, parameters),
          m2x_mode(parameters.count("m2x_mode") > 0 ? parameters["m2x_mode"].get<bool>() : false),
          diff_decode(parameters.count("diff_decode") > 0 ? parameters["diff_decode"].get<bool>() : false),
          interleaved(parameters.count("interleaved") > 0 ? parameters["interleaved"].get<bool>() : false),
          deframer_outsync_after(parameters.count("deframer_outsync_after") > 0 ? parameters["deframer_outsync_after"].get<int>() : 100)
    {
        if (parameters.count("rrc_alpha") > 0)
            d_rrc_alpha = parameters["rrc_alpha"].get<float>();
        else
            throw satdump_exception("RRC Alpha parameter must be present!");

        if (parameters.count("rrc_taps") > 0)
            d_rrc_taps = parameters["rrc_taps"].get<int>();

        if (parameters.count("pll_bw") > 0)
            d_loop_bw = parameters["pll_bw"].get<float>();
        else
            throw satdump_exception("PLL BW parameter must be present!");

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

        if (m2x_mode)
        {
            name = "METEOR LRPT OQPSK Unified Demodulator";
            MIN_SPS = 1.6;
            MAX_SPS = 2.4;
        }
        else
        {
            name = "METEOR LRPT QPSK Unified Demodulator";
        }

        // Show freq
        show_freq = true;

        // Buffers
        sym_buffer = new int8_t[BUFFER_SIZE * 4];
        thread_fifo = std::make_shared<dsp::RingBuffer<int8_t>>(1000000);

        _buffer = new int8_t[ENCODED_FRAME_SIZE + INTER_MARKER_STRIDE];
        buffer = &_buffer[INTER_MARKER_STRIDE];

        if (m2x_mode)
        {
            int d_viterbi_outsync_after = parameters["viterbi_outsync_after"].get<int>();
            float d_viterbi_ber_threasold = parameters["viterbi_ber_thresold"].get<float>();
            std::vector<phase_t> phases = {PHASE_0, PHASE_90};
            viterbin = std::make_shared<viterbi::Viterbi1_2>(d_viterbi_ber_threasold, d_viterbi_outsync_after, BUFFER_SIZE, phases, true);
            deframer = std::make_shared<deframing::BPSK_CCSDS_Deframer>(8192);

            if (interleaved)
            {
                _buffer2 = new int8_t[ENCODED_FRAME_SIZE + INTER_MARKER_STRIDE];
                buffer2 = &_buffer2[INTER_MARKER_STRIDE];
                viterbin2 = std::make_shared<viterbi::Viterbi1_2>(d_viterbi_ber_threasold, d_viterbi_outsync_after, BUFFER_SIZE, phases, true);
            }
        }
        else
        {
            viterbi = std::make_shared<viterbi::Viterbi27>(ENCODED_FRAME_SIZE / 2, viterbi::CCSDS_R2_K7_POLYS);
        }
    }

    std::vector<ModuleDataType> MeteorConcatLrptDemod::getInputTypes()
    {
        return { DATA_FILE, DATA_STREAM };
    }

    std::vector<ModuleDataType> MeteorConcatLrptDemod::getOutputTypes()
    {
        return { DATA_FILE }; // TODO - stream to decoder - live image preview?
    }

    void MeteorConcatLrptDemod::init()
    {
        BaseDemodModule::initb();

        // RRC
        rrc = std::make_shared<dsp::FIRBlock<complex_t>>(agc->output_stream, dsp::firdes::root_raised_cosine(1, final_samplerate, d_symbolrate, d_rrc_alpha, d_rrc_taps));

        // PLL
        pll = std::make_shared<dsp::CostasLoopBlock>(rrc->output_stream, d_loop_bw, 4);

        if (m2x_mode)
            delay = std::make_shared<dsp::DelayOneImagBlock>(pll->output_stream);

        // Clock recovery
        rec = std::make_shared<dsp::MMClockRecoveryBlock<complex_t>>(m2x_mode ? delay->output_stream : pll->output_stream,
            final_sps, d_clock_gain_omega, d_clock_mu, d_clock_gain_mu, d_clock_omega_relative_limit);
    }

    MeteorConcatLrptDemod::~MeteorConcatLrptDemod()
    {
        delete[] sym_buffer;
        delete[] _buffer;
        if (interleaved)
            delete[] _buffer2;
    }

    void MeteorConcatLrptDemod::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = file_source->getFilesize();
        else
            filesize = 0;

        if (output_data_type == DATA_FILE)
        {
            data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
            d_output_files.push_back(d_output_file_hint + ".cadu");
        }

        logger->info("Constellation : " + std::string(m2x_mode ? "OQPSK" : "QPSK"));
        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".soft");
        logger->info("Buffer size : %d", BUFFER_SIZE);

        time_t lastTime = 0;

        // Start
        BaseDemodModule::start();
        rrc->start();
        pll->start();
        if (m2x_mode)
            delay->start();
        rec->start();

        deframer_thread = std::thread([this]()
            {
                if (m2x_mode)
                {
                    std::shared_ptr<DeinterleaverReader> deint1, deint2;

                    if (interleaved)
                    {
                        deint1 = std::make_shared<DeinterleaverReader>();
                        deint2 = std::make_shared<DeinterleaverReader>();
                    }

                    uint8_t *viterbi_out = new uint8_t[BUFFER_SIZE * 2];
                    uint8_t *viterbi_out2 = new uint8_t[BUFFER_SIZE * 2];
                    uint8_t *frame_buffer = new uint8_t[BUFFER_SIZE * 2];

                    reedsolomon::ReedSolomon reed_solomon(reedsolomon::RS223);
                    diff::NRZMDiff diff;

                    DintSampleReader file_reader;
                    file_reader.input_function =
                        [this](int8_t* buf, size_t len) -> int
                        { return !(!thread_fifo->read(buf, len)); };

                    int noSyncsRuns = 0;

                    while (demod_should_run())
                    {
                        // Read a buffer
                        if (interleaved)
                        {
                            deint1->read_samples([&file_reader](int8_t *buf, size_t len) -> int
                                { return (bool)file_reader.read1(buf, len); },
                                buffer, 8192);
                            deint2->read_samples([&file_reader](int8_t *buf, size_t len) -> int
                                { return (bool)file_reader.read2(buf, len); },
                                buffer2, 8192);
                        }
                        else
                            thread_fifo->read(buffer, BUFFER_SIZE);

                        // Perform Viterbi decoding
                        int vitout = 0, vitout1 = 0, vitout2 = 0;

                        if (interleaved)
                        {
                            vitout1 = viterbin->work((int8_t*)buffer, BUFFER_SIZE, viterbi_out);
                            vitout2 = viterbin2->work((int8_t*)buffer2, BUFFER_SIZE, viterbi_out2);
                            if (viterbin2->getState() > viterbin->getState())
                            {
                                vitout = vitout2;
                                viterbi_ber = viterbin2->ber();
                                viterbi_lock = viterbin2->getState();
                                memcpy(viterbi_out, viterbi_out2, vitout2);
                            }
                            else
                            {
                                vitout = vitout1;
                                viterbi_ber = viterbin->ber();
                                viterbi_lock = viterbin->getState();
                            }
                        }
                        else
                        {
                            vitout = viterbin->work((int8_t*)buffer, BUFFER_SIZE, viterbi_out);
                            viterbi_ber = viterbin->ber();
                            viterbi_lock = viterbin->getState();
                        }

                        if (diff_decode) // Diff decoding if required
                            diff.decode_bits(viterbi_out, vitout);

                        // Run deframer
                        int frames = deframer->work(viterbi_out, vitout, frame_buffer);

                        for (int i = 0; i < frames; i++)
                        {
                            uint8_t* cadu = &frame_buffer[i * 1024];

                            derand_ccsds(&cadu[4], 1020);

                            // RS Correction
                            reed_solomon.decode_interlaved(&cadu[4], false, 4, errors);

                            if (errors[0] >= 0 && errors[1] >= 0 && errors[2] >= 0 && errors[3] >= 0)
                            {
                                // Write it out
                                if (output_data_type == DATA_STREAM)
                                    output_fifo->write((uint8_t*)cadu, 1024);
                                else
                                    data_out.write((char*)cadu, 1024);
                            }
                        }

                        // Force reset PLL if deframer is out of sync for too long
                        if (deframer->getState() == deframer->STATE_NOSYNC)
                        {
                            noSyncsRuns++;
                            if (noSyncsRuns >= deframer_outsync_after)
                            {
                                pll->reset();
                                noSyncsRuns = 0;
                            }
                        }
                        else
                        {
                            noSyncsRuns = 0;
                        }

                        // Update module stats
                        module_stats["deframer_lock"] = deframer->getState() == deframer->STATE_SYNCED;
                        module_stats["viterbi_ber"] = viterbi_ber;
                        module_stats["viterbi_lock"] = viterbi_lock;
                        module_stats["rs_avg"] = (errors[0] + errors[1] + errors[2] + errors[3]) / 4;
                    }

                    delete[] viterbi_out;
                    delete[] viterbi_out2;
                    delete[] frame_buffer;
                }
                else
                {
                    // Correlator
                    Correlator correlator(QPSK, diff_decode ? 0xfc4ef4fd0cc2df89 : 0xfca2b63db00d9794);

                    // Viterbi, rs, etc
                    reedsolomon::ReedSolomon rs(reedsolomon::RS223);

                    // Other buffers
                    uint8_t frameBuffer[FRAME_SIZE];

                    phase_t phase;
                    bool swap;

                    // Vectors are inverted
                    diff::NRZMDiff diff;

                    //TODO: NOSYNC RUNS CHECK

                    while (demod_should_run())
                    {
                        // Read a buffer
                        thread_fifo->read(buffer, ENCODED_FRAME_SIZE);

                        int pos = correlator.correlate((int8_t*)buffer, phase, swap, cor, ENCODED_FRAME_SIZE);

                        locked = pos == 0; // Update locking state

                        if (pos != 0 && pos < ENCODED_FRAME_SIZE) // Safety
                        {
                            std::memmove(buffer, &buffer[pos], ENCODED_FRAME_SIZE - pos);
                            thread_fifo->read(&buffer[ENCODED_FRAME_SIZE - pos], pos);
                        }

                        // Correct phase ambiguity
                        rotate_soft(buffer, ENCODED_FRAME_SIZE, phase, swap);

                        // Viterbi
                        viterbi->work((int8_t*)buffer, frameBuffer);

                        if (diff_decode)
                            diff.decode(frameBuffer, FRAME_SIZE);

                        // Derandomize that frame
                        derand_ccsds(&frameBuffer[4], FRAME_SIZE - 4);

                        // There is a VERY rare edge case where CADUs end up inverted... Better cover it to be safe
                        if (frameBuffer[9] == 0xFF)
                        {
                            for (int i = 0; i < FRAME_SIZE; i++)
                                frameBuffer[i] ^= 0xFF;
                        }

                        // RS Correction
                        rs.decode_interlaved(&frameBuffer[4], false, 4, errors);

                        // Write it out if it's not garbage
                        if (errors[0] >= 0 && errors[1] >= 0 && errors[2] >= 0 && errors[3] >= 0)
                        {
                            data_out.put(0x1a);
                            data_out.put(0xcf);
                            data_out.put(0xfc);
                            data_out.put(0x1d);
                            data_out.write((char*)&frameBuffer[4], FRAME_SIZE - 4);
                        }

                        // Update module stats
                        module_stats["correlator_lock"] = locked;
                        module_stats["viterbi_ber"] = viterbi->ber();
                        module_stats["rs_avg"] = (errors[0] + errors[1] + errors[2] + errors[3]) / 4;
                    }
                }
            });

        int dat_size = 0;
        while (demod_should_run())
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
                sym_buffer[i * 2] = clamp(rec->output_stream->readBuf[i].real * 100);
                sym_buffer[i * 2 + 1] = clamp(rec->output_stream->readBuf[i].imag * 100);
            }

            thread_fifo->write(sym_buffer, dat_size * 2);
            rec->output_stream->flush();

            // Update module stats
            module_stats["snr"] = snr;
            module_stats["peak_snr"] = peak_snr;
            module_stats["freq"] = display_freq;

            if (input_data_type == DATA_FILE)
                progress = file_source->getPosition();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%, SNR : " + std::to_string(snr) + "dB," + " Peak SNR: " + std::to_string(peak_snr) + "dB");
                if (m2x_mode)
                {
                    std::string viterbi_state = viterbi_lock == 0 ? "NOSYNC" : "SYNCED";
                    std::string deframer_state = deframer->getState() == deframer->STATE_NOSYNC ? "NOSYNC" : (deframer->getState() == deframer->STATE_SYNCING ? "SYNCING" : "SYNCED");
                    logger->info("Viterbi : " + viterbi_state + " BER : " + std::to_string(viterbi_ber) + ", Deframer : " + deframer_state);
                }
                else
                {
                    std::string lock_state = locked ? "SYNCED" : "NOSYNC";
                    logger->info("Viterbi BER : " + std::to_string(viterbi->ber() * 100) + "%%, Lock : " + lock_state);
                }
            }
        }

        logger->info("Demodulation finished");

        if (input_data_type == DATA_FILE)
            stop();

        if (deframer_thread.joinable())
            deframer_thread.join();
        if (output_data_type == DATA_FILE)
            data_out.close();
    }

    void MeteorConcatLrptDemod::stop()
    {
        // Stop
        BaseDemodModule::stop();

        rrc->stop();
        pll->stop();
        if (m2x_mode)
            delay->stop();
        rec->stop();
        rec->output_stream->stopReader();
    }

    void MeteorConcatLrptDemod::drawUI(bool window)
    {
        ImGui::Begin(name.c_str(), NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        constellation.draw();
        ImGui::EndGroup();
        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            if (m2x_mode)
            {
                float ber = viterbi_ber;

                ImGui::Button("Viterbi", { 200 * ui_scale, 20 * ui_scale });
                {
                    ImGui::Text("State : ");

                    ImGui::SameLine();

                    if (viterbi_lock == 0)
                        ImGui::TextColored(style::theme.red, "NOSYNC");
                    else
                        ImGui::TextColored(style::theme.green, "SYNCED");

                    ImGui::Text("BER   : ");
                    ImGui::SameLine();
                    ImGui::TextColored(viterbi_lock == 0 ? style::theme.red : style::theme.green, UITO_C_STR(ber));

                    std::memmove(&ber_history[0], &ber_history[1], (200 - 1) * sizeof(float));
                    ber_history[200 - 1] = ber;

                    widgets::ThemedPlotLines(style::theme.plot_bg.Value, "", ber_history, IM_ARRAYSIZE(ber_history), 0, "", 0.0f, 1.0f,
                        ImVec2(200 * ui_scale, 50 * ui_scale));
                }

                ImGui::Spacing();

                ImGui::Button("Deframer", { 200 * ui_scale, 20 * ui_scale });
                {
                    ImGui::Text("State : ");

                    ImGui::SameLine();

                    if (deframer->getState() == deframer->STATE_NOSYNC)
                        ImGui::TextColored(style::theme.red, "NOSYNC");
                    else if (deframer->getState() == deframer->STATE_SYNCING)
                        ImGui::TextColored(style::theme.orange, "SYNCING");
                    else
                        ImGui::TextColored(style::theme.green, "SYNCED");
                }

                ImGui::Spacing();

                ImGui::Button("Reed-Solomon", { 200 * ui_scale, 20 * ui_scale });
                {
                    ImGui::Text("RS    : ");
                    for (int i = 0; i < 4; i++)
                    {
                        ImGui::SameLine();

                        if (errors[i] == -1)
                            ImGui::TextColored(style::theme.red, "%i ", i);
                        else if (errors[i] > 0)
                            ImGui::TextColored(style::theme.orange, "%i ", i);
                        else
                            ImGui::TextColored(style::theme.green, "%i ", i);
                    }
                }
            }
            else
            {
                float ber = viterbi->ber();

                ImGui::Button("Correlator", { 200 * ui_scale, 20 * ui_scale });
                {
                    ImGui::Text("Corr  : ");
                    ImGui::SameLine();
                    ImGui::TextColored(locked ? style::theme.green : style::theme.orange, UITO_C_STR(cor));

                    std::memmove(&cor_history[0], &cor_history[1], (200 - 1) * sizeof(float));
                    cor_history[200 - 1] = cor;

                    widgets::ThemedPlotLines(style::theme.plot_bg.Value, "", cor_history, IM_ARRAYSIZE(cor_history), 0, "", 40.0f, 64.0f,
                        ImVec2(200 * ui_scale, 50 * ui_scale));
                }

                ImGui::Spacing();

                ImGui::Button("Viterbi", { 200 * ui_scale, 20 * ui_scale });
                {
                    ImGui::Text("BER   : ");
                    ImGui::SameLine();
                    ImGui::TextColored(ber < 0.22 ? style::theme.green : style::theme.red, UITO_C_STR(ber));

                    std::memmove(&ber_history[0], &ber_history[1], (200 - 1) * sizeof(float));
                    ber_history[200 - 1] = ber;

                    widgets::ThemedPlotLines(style::theme.plot_bg.Value, "", ber_history, IM_ARRAYSIZE(ber_history), 0, "", 0.0f, 1.0f,
                        ImVec2(200 * ui_scale, 50 * ui_scale));
                }

                ImGui::Spacing();

                ImGui::Button("Reed-Solomon", { 200 * ui_scale, 20 * ui_scale });
                {
                    ImGui::Text("RS    : ");
                    for (int i = 0; i < 4; i++)
                    {
                        ImGui::SameLine();

                        if (errors[i] == -1)
                            ImGui::TextColored(style::theme.red, "%i ", i);
                        else if (errors[i] > 0)
                            ImGui::TextColored(style::theme.orange, "%i ", i);
                        else
                            ImGui::TextColored(style::theme.green, "%i ", i);
                    }
                }
            }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

        ImGui::End();
    }

    std::string MeteorConcatLrptDemod::getID()
    {
        return "meteor_concat_lrpt_demod";
    }

    std::vector<std::string> MeteorConcatLrptDemod::getParameters()
    {
        std::vector<std::string> params = { "rrc_alpha", "rrc_taps", "pll_bw", "clock_gain_omega", "clock_mu", "clock_gain_mu", "clock_omega_relative_limit" };
        params.insert(params.end(), BaseDemodModule::getParameters().begin(), BaseDemodModule::getParameters().end());
        return params;
    }

    std::shared_ptr<ProcessingModule> MeteorConcatLrptDemod::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<MeteorConcatLrptDemod>(input_file, output_file_hint, parameters);
    }
}