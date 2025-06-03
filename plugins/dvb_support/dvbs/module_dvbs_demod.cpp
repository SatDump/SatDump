#include "module_dvbs_demod.h"
#include "common/dsp/filter/firdes.h"
#include "common/widgets/themed_widgets.h"
#include "dvbs/dvbs_defines.h"
#include "dvbs/dvbs_interleaving.h"
#include "dvbs/dvbs_reedsolomon.h"
#include "dvbs/dvbs_scrambling.h"
#include "logger.h"

namespace satdump
{
    namespace pipeline
    {
        namespace dvb
        {
            DVBSDemodModule::DVBSDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
                : BaseDemodModule(input_file, output_file_hint, parameters), viterbi(0.19, 50, VIT_BUF_SIZE, {PHASE_0, PHASE_90})
            {
                if (parameters.count("rrc_alpha") > 0)
                    d_rrc_alpha = parameters["rrc_alpha"].get<float>();

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

                name = "DVB-S Demodulator (WIP)";

                // Show freq
                show_freq = true;
            }

            void DVBSDemodModule::init()
            {
                BaseDemodModule::initb();

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
                if (d_parameters.contains("fast_tssync"))
                    def->d_fast_deframer = d_parameters["fast_tssync"];
            }

            DVBSDemodModule::~DVBSDemodModule() {}

            void DVBSDemodModule::process()
            {
                if (input_data_type == DATA_FILE)
                    filesize = file_source->getFilesize();
                else
                    filesize = 0;

                if (output_data_type == DATA_FILE)
                {
                    data_out = std::ofstream(d_output_file_hint + ".ts", std::ios::binary);
                    d_output_file = d_output_file_hint + ".ts";
                }

                logger->info("Using input baseband " + d_input_file);
                logger->info("Demodulating to " + d_output_file_hint + ".ts");
                logger->info("Buffer size : %d", d_buffer_size);

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

                int failed_rs_nums = 0;

                int dat_size = 0;
                while (demod_should_run())
                {
                    // Handle outputs
                    // Get rate
                    rate = "";
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

                    if (input_data_type == DATA_FILE)
                        progress = file_source->getPosition();
                    if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                    {
                        lastTime = time(NULL);

                        std::string viterbi_l = std::string(viterbi.getState() == 0 ? "NOSYNC" : "SYNC") + " " + rate;
                        logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%, SNR : " + std::to_string(snr) + "dB, Viterbi : " + viterbi_l +
                                     ", Deframer Sync : " + std::to_string(def->isSynced()) + ", Peak SNR: " + std::to_string(peak_snr) + "dB");
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

                        for (int ii = 0; ii < 8; ii++)
                            errors[ii] = reed_solomon.decode(&deinterleaved_frame[204 * ii]);

                        scrambler.descramble(deinterleaved_frame);

                        bool rs_entirely_failed = true;
                        for (int ii = 0; ii < 8; ii++)
                        {
                            if (errors[ii] == -1)
                                deinterleaved_frame[204 * ii + 1] |= 0b10000000;
                            else
                                rs_entirely_failed = false;

                            if (output_data_type == DATA_FILE)
                                data_out.write((char *)&deinterleaved_frame[204 * ii], 188);
                            else
                                output_fifo->write((uint8_t *)&deinterleaved_frame[204 * ii], 188);
                        }

                        if (rs_entirely_failed)
                        {
                            failed_rs_nums++;
                            if (failed_rs_nums == 20)
                            {
                                viterbi.reset();
                                def->reset();
                                failed_rs_nums = 0;
                            }
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

            nlohmann::json DVBSDemodModule::getModuleStats()
            {
                nlohmann::json v; // auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
                v["progress"] = ((double)progress / (double)filesize);
                v["snr"] = snr;
                v["peak_snr"] = peak_snr;
                v["freq"] = display_freq;
                v["viterbi_ber"] = viterbi.ber();
                v["viterbi_lock"] = viterbi.getState();
                v["viterbi_rate"] = rate;
                if (def->d_fast_deframer)
                    v["deframer_synced"] = def->isSynced();
                v["rs_avg"] = (errors[0] + errors[1] + errors[2] + errors[3] + errors[4] + errors[5] + errors[6] + errors[7]) / 8;
                return v;
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
                        ImGui::TextColored(style::theme.orange, "%.0f Hz", display_freq);
                    }
                    snr_plot.draw(snr, peak_snr);
                    if (!d_is_streaming_input)
                        if (ImGui::Checkbox("Show FFT", &show_fft))
                            fft_splitter->set_enabled("fft", show_fft);
                }
                ImGui::EndGroup();
                ImGui::SameLine();
                ImGui::BeginGroup();
                {
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
                            ImGui::TextColored(style::theme.red, "NOSYNC");
                        else
                            ImGui::TextColored(style::theme.green, "SYNCED %s", rate.c_str());

                        ImGui::Text("BER   : ");
                        ImGui::SameLine();
                        ImGui::TextColored(viterbi.getState() == 0 ? style::theme.red : style::theme.green, UITO_C_STR(ber));

                        std::memmove(&ber_history[0], &ber_history[1], (200 - 1) * sizeof(float));
                        ber_history[200 - 1] = ber;

                        widgets::ThemedPlotLines(style::theme.plot_bg.Value, "", ber_history, IM_ARRAYSIZE(ber_history), 0, "", 0.0f, 1.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
                    }

                    if (def->d_fast_deframer)
                    {
                        ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});

                        ImGui::Spacing();

                        ImGui::Text("State : ");
                        ImGui::SameLine();
                        if (def->isSynced())
                            ImGui::TextColored(style::theme.green, "SYNCED");
                        else
                            ImGui::TextColored(style::theme.red, "NOSYNC");
                    }

                    ImGui::Spacing();

                    ImGui::Button("Reed-Solomon", {200 * ui_scale, 20 * ui_scale});
                    {
                        ImGui::Text("RS    : ");
                        for (int i = 0; i < 8; i++)
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
                ImGui::EndGroup();

                if (!d_is_streaming_input)
                    ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

                drawStopButton();

                ImGui::End();

                drawFFT();
            }

            std::string DVBSDemodModule::getID() { return "dvbs_demod"; }

            // std::vector<std::string> DVBSDemodModule::getParameters()
            // {
            //     std::vector<std::string> params = {"rrc_alpha", "rrc_taps", "pll_bw", "clock_gain_omega", "clock_mu", "clock_gain_mu", "clock_omega_relative_limit"};
            //     params.insert(params.end(), BaseDemodModule::getParameters().begin(), BaseDemodModule::getParameters().end());
            //     return params;
            // } TODOREWORK

            std::shared_ptr<ProcessingModule> DVBSDemodModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            {
                return std::make_shared<DVBSDemodModule>(input_file, output_file_hint, parameters);
            }
        } // namespace dvb
    } // namespace pipeline
} // namespace satdump