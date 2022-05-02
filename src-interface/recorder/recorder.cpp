#include "recorder.h"
#include "core/config.h"
#include "common/utils.h"
#include "core/style.h"
#include "logger.h"

namespace satdump
{
    RecorderApplication::RecorderApplication()
        : Application("recorder")
    {
        dsp::registerAllSources();

        sources = dsp::getAllAvailableSources();

        for (dsp::SourceDescriptor src : sources)
        {
            logger->debug("Device " + src.name);
            sdr_select_string += src.name + '\0';
        }

        source_ptr = dsp::getSourceFromDescriptor(sources[0]);
        source_ptr->open();
        source_ptr->set_frequency(100e6);

        splitter = std::make_shared<dsp::SplitterBlock>(source_ptr->output_stream);
        splitter->set_output_2nd(false);

        fft = std::make_shared<dsp::FFTBlock>(splitter->output_stream);
        fft->set_fft_settings(fft_size);
        fft->start();

        file_sink = std::make_shared<dsp::FileSinkBlock>(splitter->output_stream_2);
        file_sink->set_output_sample_type(dsp::COMPLEX_FLOAT_32);
        file_sink->start();

        fft_plot = std::make_shared<widgets::FFTPlot>(fft->output_stream->writeBuf, fft_size, -10, 20, 10);
        waterfall_plot = std::make_shared<widgets::WaterfallPlot>(fft->output_stream->writeBuf, fft_size, 2000);
    }

    RecorderApplication::~RecorderApplication()
    {
    }

    void RecorderApplication::drawUI()
    {
        ImVec2 recorder_size = ImGui::GetContentRegionAvail();
        ImGui::BeginGroup();

        ImGui::BeginChild("RecorderChildPanel", {float(recorder_size.x * 0.20), float(recorder_size.y)}, false);
        {
            if (ImGui::CollapsingHeader("Device"))
            {
                if (is_started)
                    style::beginDisabled();
                if (ImGui::Combo("Source", &sdr_select_id, sdr_select_string.c_str()))
                {
                    source_ptr = getSourceFromDescriptor(sources[sdr_select_id]);
                    source_ptr->open();
                    source_ptr->set_frequency(100e6);
                }
                if (is_started)
                    style::endDisabled();

                if (ImGui::InputDouble("MHz", &frequency_mhz))
                    source_ptr->set_frequency(frequency_mhz * 1e6);

                source_ptr->drawControlUI();

                if (!is_started)
                {
                    if (ImGui::Button("Start"))
                    {
                        source_ptr->start();
                        splitter->input_stream = source_ptr->output_stream;
                        splitter->start();
                        is_started = true;
                    }
                }
                else
                {
                    if (ImGui::Button("Stop"))
                    {
                        splitter->stop_tmp();
                        source_ptr->stop();
                        is_started = false;
                    }
                }
            }

            if (ImGui::CollapsingHeader("FFT"))
            {
                ImGui::SliderFloat("FFT Max", &fft_plot->scale_max, -80, 80);
                ImGui::SliderFloat("FFT Min", &fft_plot->scale_min, -80, 80);
            }

            if (fft_plot->scale_max < fft_plot->scale_min)
            {
                fft_plot->scale_min = waterfall_plot->scale_min;
                fft_plot->scale_max = waterfall_plot->scale_max;
            }
            else if (fft_plot->scale_min > fft_plot->scale_max)
            {
                fft_plot->scale_min = waterfall_plot->scale_min;
                fft_plot->scale_max = waterfall_plot->scale_max;
            }
            else
            {
                waterfall_plot->scale_min = fft_plot->scale_min;
                waterfall_plot->scale_max = fft_plot->scale_max;
            }

            if (ImGui::CollapsingHeader("Processing"))
            {
            }

            if (ImGui::CollapsingHeader("Recording"))
            {
                if (is_recording)
                    style::beginDisabled();
                if (ImGui::Combo("Format", &select_sample_format, "f32\0"
                                                                  "s16\0"
                                                                  "s8\0"))
                {
                    if (select_sample_format == 0)
                        file_sink->set_output_sample_type(dsp::COMPLEX_FLOAT_32);
                    else if (select_sample_format == 1)
                        file_sink->set_output_sample_type(dsp::INTEGER_16);
                    else if (select_sample_format == 2)
                        file_sink->set_output_sample_type(dsp::INTEGER_8);
                }
                if (is_recording)
                    style::endDisabled();

                if (file_sink->get_written() < 1e9)
                    ImGui::Text("Size : %.2f MB", file_sink->get_written() / 1e6);
                else
                    ImGui::Text("Size : %.2f GB", file_sink->get_written() / 1e9);
                ImGui::Text("File : %s", recorder_filename.c_str());

                ImGui::Spacing();

                if (!is_recording)
                    ImGui::TextColored(ImColor(255, 0, 0), "IDLE");
                else
                    ImGui::TextColored(ImColor(0, 255, 0), "RECORDING");

                ImGui::Spacing();

                if (!is_recording)
                {
                    if (ImGui::Button("Start###startrecording"))
                    {
                        splitter->set_output_2nd(true);

                        const time_t timevalue = time(0);
                        std::tm *timeReadable = gmtime(&timevalue);
                        std::string timestamp =
                            (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                            (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + "-" +
                            (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));

                        std::string filename = "./" + timestamp + "_" + std::to_string((long)source_ptr->get_samplerate()) + "SPS_" +
                                               std::to_string(long(frequency_mhz * 1e6)) + "Hz";

                        recorder_filename = file_sink->start_recording(filename);

                        logger->info("Recording to " + recorder_filename);

                        is_recording = true;
                    }
                }
                else
                {
                    if (ImGui::Button("Stop##stoprecording"))
                    {
                        file_sink->stop_recording();
                        splitter->set_output_2nd(false);
                        recorder_filename = "";
                        is_recording = false;
                    }
                }
            }
        }
        ImGui::EndChild();
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::BeginChild("RecorderFFT", {float(recorder_size.x * 0.80), float(recorder_size.y)}, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            fft_plot->draw({float(recorder_size.x * 0.80 - 4), float(recorder_size.y * 0.3)});
            waterfall_plot->draw({float(recorder_size.x * 0.80 - 4), float(recorder_size.y * 0.7) * 4});
        }
        ImGui::EndChild();
        ImGui::EndGroup();
    }
};