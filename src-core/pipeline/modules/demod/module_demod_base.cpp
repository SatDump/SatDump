#include "module_demod_base.h"
#include "common/dsp_source_sink/format_notated.h"
#include "core/config.h"
#include "imgui/imgui.h"
#include "logger.h"

namespace satdump
{
    namespace pipeline
    {
        namespace demod
        {
            BaseDemodModule::BaseDemodModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
                : ProcessingModule(input_file, output_file_hint, parameters), constellation(100.0f / 127.0f, 100.0f / 127.0f, demod_constellation_size)
            {
                // Parameters parsing
                if (parameters.count("samplerate") > 0)
                    d_samplerate = parameters["samplerate"].get<long>();
                else
                    throw satdump_exception("Samplerate parameter must be present!");

                if (parameters.count("buffer_size") > 0)
                    d_buffer_size = parameters["buffer_size"].get<long>();
                else
                    d_buffer_size = std::min<int>(dsp::STREAM_BUFFER_SIZE, std::max<int>(8192 + 1, d_samplerate / 200));

                if (parameters.count("symbolrate") > 0)
                    d_symbolrate = parameters["symbolrate"].get<long>();

                if (parameters.count("agc_rate") > 0)
                    d_agc_rate = parameters["agc_rate"].get<float>();

                if (parameters.count("dc_block") > 0)
                    d_dc_block = parameters["dc_block"].get<bool>();

                if (parameters.count("freq_shift") > 0)
                    d_frequency_shift = parameters["freq_shift"].get<long>();

                if (parameters.count("iq_swap") > 0)
                    d_iq_swap = parameters["iq_swap"].get<bool>();

                /////////////////////
                if (parameters.count("enable_doppler") > 0)
                    d_doppler_enable = parameters["enable_doppler"].get<bool>();

                if (parameters.count("doppler_alpha") > 0)
                    d_doppler_alpha = parameters["doppler_alpha"].get<float>();
                /////////////////////

                if (parameters.count("dump_intermediate") > 0)
                    d_dump_intermediate = parameters["dump_intermediate"].get<std::string>();

                snr = 0;
                peak_snr = 0;

                showWaterfall = satdump::satdump_cfg.main_cfg["user_interface"]["show_waterfall_demod_fft"]["value"].get<bool>();
            }

            void BaseDemodModule::initb(bool resample_here)
            {
                if (d_parameters.contains("min_sps"))
                    MIN_SPS = d_parameters["min_sps"].get<float>();
                if (d_parameters.contains("max_sps"))
                    MAX_SPS = d_parameters["max_sps"].get<float>();

                float input_sps = (float)d_samplerate / (float)d_symbolrate; // Compute input SPS
                resample = input_sps > MAX_SPS || input_sps < MIN_SPS;       // If SPS is out of allowed range, we resample

                int range = pow(10, (std::to_string(int(d_symbolrate)).size() - 1)); // Avoid complex resampling

                final_samplerate = d_samplerate;

                if (d_parameters.count("custom_samplerate") > 0)
                    final_samplerate = d_parameters["custom_samplerate"].get<long>();
                else if (MAX_SPS == MIN_SPS)
                    final_samplerate = d_symbolrate * MAX_SPS;
                else if (input_sps > MAX_SPS)
                    final_samplerate = resample ? (round(d_symbolrate / range) * range) * MAX_SPS : d_samplerate; // Get the final samplerate we'll be working with
                else if (input_sps < MIN_SPS)
                    final_samplerate = resample ? d_symbolrate * MIN_SPS : d_samplerate; // Get the final samplerate we'll be working with

                float decimation_factor = d_samplerate / final_samplerate; // Decimation factor to rescale our input buffer

                if (resample)
                    d_buffer_size *= ceil(decimation_factor);
                if (d_buffer_size > 8192 * 20)
                    d_buffer_size = 8192 * 20;

                final_sps = final_samplerate / (float)d_symbolrate;

                logger->debug("Input SPS : %f", input_sps);
                logger->debug("Resample : " + std::to_string(resample));
                logger->debug("Samplerate : %f", final_samplerate);
                logger->debug("Dec factor : %f", decimation_factor);
                logger->debug("Final SPS : %f", final_sps);

                // clang-format off
                if (input_sps < 1.0)
                    // Formats according to the symbol rate (0.001 Msps would be awful to read)
                    throw satdump_exception("Your sampling rate is too low! Minimum: " +
                        (
                        d_symbolrate > 1e6
                        ? std::to_string(d_symbolrate / 1e6) + " Msps"
                        : std::to_string(d_symbolrate / 1e3) + " ksps"
                        )
                    );
                // clang-format on

                // Init DSP Blocks
                if (input_data_type == DATA_FILE)
                    file_source = std::make_shared<dsp::FileSourceBlock>(d_input_file, d_parameters["baseband_format"].get<std::string>(), d_buffer_size, d_iq_swap);

                if (d_dc_block)
                    dc_blocker = std::make_shared<dsp::CorrectIQBlock<complex_t>>(input_data_type == DATA_DSP_STREAM ? input_stream : file_source->output_stream);

                // Cleanup things a bit
                std::shared_ptr<dsp::stream<complex_t>> input_data = d_dc_block ? dc_blocker->output_stream : (input_data_type == DATA_DSP_STREAM ? input_stream : file_source->output_stream);

                if (d_frequency_shift != 0)
                    freq_shift = std::make_shared<dsp::FreqShiftBlock>(input_data, d_samplerate, d_frequency_shift);

                if (d_doppler_enable)
                {
                    double frequency = -1;
                    if (d_parameters.count("satellite_frequency"))
                        frequency = d_parameters["satellite_frequency"].get<double>();
                    else
                        throw satdump_exception("Satellite Frequency is required for doppler correction!");

                    if (d_frequency_shift != 0)
                        frequency += d_frequency_shift;

                    int norad = -1;
                    if (d_parameters.count("satellite_norad"))
                        norad = d_parameters["satellite_norad"].get<double>();
                    else
                        throw satdump_exception("Satellite NORAD is required for doppler correction!");

                    // QTH, with a way to override it
                    double qth_lon = 0, qth_lat = 0, qth_alt = 0;
                    try
                    {
                        qth_lon = satdump::satdump_cfg.getValueFromSatDumpGeneral<double>("qth_lon");
                        qth_lat = satdump::satdump_cfg.getValueFromSatDumpGeneral<double>("qth_lat");
                        qth_alt = satdump::satdump_cfg.getValueFromSatDumpGeneral<double>("qth_alt");
                    }
                    catch (std::exception &)
                    {
                    }

                    if (d_parameters.count("qth_lon"))
                        qth_lon = d_parameters["qth_lon"].get<double>();
                    if (d_parameters.count("qth_lat"))
                        qth_lat = d_parameters["qth_lat"].get<double>();
                    if (d_parameters.count("qth_alt"))
                        qth_alt = d_parameters["qth_alt"].get<double>();

                    doppler_shift = std::make_shared<dsp::DopplerCorrectBlock>(d_frequency_shift != 0 ? freq_shift->output_stream : input_data, d_samplerate, d_doppler_alpha, frequency, norad,
                                                                               qth_lon, qth_lat, qth_alt);
                    if (input_data_type == DATA_FILE)
                    {
                        if (d_parameters.count("start_timestamp") > 0)
                            doppler_shift->start_time = d_parameters["start_timestamp"].get<double>();
                        else
                        {
                            logger->error("Start Timestamp is required for doppler correction! Disabling doppler.");
                            doppler_shift.reset();
                            d_doppler_enable = false;
                        }
                    }
                }

                std::shared_ptr<dsp::stream<complex_t>> input_data_final = d_doppler_enable ? doppler_shift->output_stream : (d_frequency_shift != 0 ? freq_shift->output_stream : input_data);

                if (input_data_type == DATA_FILE)
                {
                    fft_splitter = std::make_shared<dsp::SplitterBlock>(input_data_final);
                    fft_splitter->add_output("fft");
                    fft_splitter->set_enabled("fft", show_fft);

                    if (d_dump_intermediate != "")
                    {
                        fft_splitter->add_output("intermediate");
                        fft_splitter->set_enabled("intermediate", true);
                        intermediate_file_sink = std::make_shared<dsp::FileSinkBlock>(fft_splitter->get_output("intermediate"));
                    }

                    fft_proc = std::make_shared<dsp::FFTPanBlock>(fft_splitter->get_output("fft"));
                    fft_proc->set_fft_settings(8192, final_samplerate, 120);
                    fft_proc->avg_num = 10;
                    fft_plot = std::make_shared<widgets::FFTPlot>(fft_proc->output_stream->writeBuf, 8192, -10, 20, 10);
                    waterfall_plot = std::make_shared<widgets::WaterfallPlot>(8192, 500);
                    waterfall_plot->set_rate(120, 10);
                    fft_proc->on_fft = [this](float *v) { waterfall_plot->push_fft(v); };
                }

                std::shared_ptr<dsp::stream<complex_t>> input_data_final_fft = input_data_type == DATA_FILE ? fft_splitter->output_stream : input_data_final;

                // Init resampler if required
                if (resample && resample_here)
                    resampler = std::make_shared<dsp::SmartResamplerBlock<complex_t>>(input_data_final_fft, final_samplerate, d_samplerate);

                // AGC
                agc = std::make_shared<dsp::AGCBlock<complex_t>>((resample && resample_here) ? resampler->output_stream : input_data_final_fft, d_agc_rate, 1.0f, 1.0f, 65536);
            }

            std::vector<ModuleDataType> BaseDemodModule::getInputTypes() { return {DATA_FILE, DATA_DSP_STREAM}; }

            std::vector<ModuleDataType> BaseDemodModule::getOutputTypes() { return {DATA_FILE, DATA_STREAM}; }

            BaseDemodModule::~BaseDemodModule() {}

            void BaseDemodModule::start()
            {
                // Start
                if (input_data_type == DATA_FILE)
                    file_source->start();
                if (d_dc_block)
                    dc_blocker->start();
                if (d_frequency_shift != 0)
                    freq_shift->start();
                if (d_doppler_enable)
                    doppler_shift->start();
                if (input_data_type == DATA_FILE)
                    fft_splitter->start();
                if (input_data_type == DATA_FILE && d_dump_intermediate != "")
                {
                    intermediate_file_sink->start();
                    intermediate_file_sink->set_output_sample_type(d_dump_intermediate);
                    std::string int_file = d_output_file_hint + "_" + std::to_string((uint64_t)d_samplerate) + "_intermediate_iq";
                    logger->trace("Recording intermediate to " + int_file);
                    intermediate_file_sink->start_recording(int_file, d_samplerate);
                }
                if (input_data_type == DATA_FILE)
                    fft_proc->start();
                if (resample && resampler)
                    resampler->start();
                agc->start();
            }

            void BaseDemodModule::stop()
            {
                // Stop
                if (input_data_type == DATA_FILE)
                    file_source->stop();
                if (d_dc_block)
                    dc_blocker->stop();
                if (d_frequency_shift != 0)
                    freq_shift->stop();
                if (d_doppler_enable)
                    doppler_shift->stop();
                if (input_data_type == DATA_FILE)
                    fft_splitter->stop();
                if (input_data_type == DATA_FILE && d_dump_intermediate != "")
                {
                    intermediate_file_sink->stop_recording();
                    intermediate_file_sink->stop();
                }
                if (input_data_type == DATA_FILE)
                    fft_proc->stop();
                if (resample && resampler)
                    resampler->stop();
                agc->stop();
            }

            void BaseDemodModule::drawUI(bool window)
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
                        ImGui::TextColored(style::theme.orange, "%s", format_notated(display_freq, "Hz", 4).c_str());
                    }
                    snr_plot.draw(snr, peak_snr);
                    if (!d_is_streaming_input)
                        if (ImGui::Checkbox("Show FFT", &show_fft))
                            fft_splitter->set_enabled("fft", show_fft);
                }
                ImGui::EndGroup();

                if (!d_is_streaming_input)
                    ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

                drawStopButton();

                ImGui::End();

                drawFFT();
            }

            void BaseDemodModule::drawFFT()
            {
                if (show_fft && !d_is_streaming_input)
                {
                    ImGui::SetNextWindowSize({400 * (float)ui_scale, (float)(showWaterfall ? 400 : 200) * (float)ui_scale});
                    if (ImGui::Begin("Baseband FFT", NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize))
                    {
                        fft_plot->draw({float(ImGui::GetWindowSize().x - 0), float(ImGui::GetWindowSize().y - 40 * ui_scale) * float(showWaterfall ? 0.5 : 1.0)});

                        // Find "actual" left edge of FFT, before frequency shift.
                        // Inset by 10% (819), then account for > 100% freq shifts via modulo
                        int pos = (abs((float)d_frequency_shift / (float)d_samplerate) * (float)8192) + 819;
                        pos %= 8192;

                        // Compute min and max of the middle 80% of original baseband
                        float min = 1000;
                        float max = -1000;
                        for (int i = 0; i < 6554; i++) // 8192 * 80% = 6554
                        {
                            if (fft_proc->output_stream->writeBuf[pos] < min)
                                min = fft_proc->output_stream->writeBuf[pos];
                            if (fft_proc->output_stream->writeBuf[pos] > max)
                                max = fft_proc->output_stream->writeBuf[pos];
                            pos++;
                            if (pos >= 8192)
                                pos = 0;
                        }

                        waterfall_plot->scale_min = fft_plot->scale_min = fft_plot->scale_min * 0.99 + min * 0.01;
                        waterfall_plot->scale_max = fft_plot->scale_max = fft_plot->scale_max * 0.99 + max * 0.01;

                        if (showWaterfall)
                            waterfall_plot->draw({ImGui::GetWindowSize().x - 0, (float)(ImGui::GetWindowSize().y - 45 * ui_scale) / 2});
                    }

                    ImGui::End();
                }
            }

            void BaseDemodModule::drawStopButton()
            {
                if (input_data_type != DATA_FILE)
                    return;

                if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
                {
                    ImGuiStyle &style = ImGui::GetStyle();
                    ImVec2 cur_pos = ImGui::GetCursorPos();
                    cur_pos.x = ImGui::GetContentRegionMax().x - ImGui::CalcTextSize("Abort").x - style.FramePadding.x * 2.0f;
                    cur_pos.y -= 20.0f * ui_scale + style.ItemSpacing.y;
                    ImGui::SetCursorPos(cur_pos);
                    ImGui::PushStyleColor(ImGuiCol_Button, style::theme.red.Value);
                    if (ImGui::Button("Abort##demodstop", ImVec2(0.0f, 20.0f * ui_scale)))
                        demod_should_stop = true;
                    ImGui::PopStyleColor();
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("This Abort button will simulate the \ndemodulation being finished. \nProcessing will carry on!");
                }
            }

            // TODOREWORK    std::vector<std::string> BaseDemodModule::getParameters() { return {"samplerate", "symbolrate", "agc_rate", "iq_swap", "buffer_size", "dc_block", "baseband_format"}; }
        } // namespace demod
    } // namespace pipeline
} // namespace satdump