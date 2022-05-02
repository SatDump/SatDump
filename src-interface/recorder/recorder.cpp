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

        fft = std::make_shared<dsp::FFTBlock>(source_ptr->output_stream);
        fft->set_fft_settings(fft_size);

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
                        fft->input_stream = source_ptr->output_stream;
                        fft->start();
                        is_started = true;
                    }
                }
                else
                {
                    if (ImGui::Button("Stop"))
                    {
                        fft->stop();
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
        }
        ImGui::EndChild();
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::BeginChild("RecorderFFT", {float(recorder_size.x * 0.80), float(recorder_size.y)}, false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            fft_plot->draw({float(recorder_size.x * 0.80 - 4), float(recorder_size.y * 0.3)});
            /*
            float min = 1000;
            for (int i = 0; i < fft_size; i++)
                if (fft->output_stream->writeBuf[i] < min)
                    min = fft->output_stream->writeBuf[i];
            float max = -1000;
            for (int i = 0; i < fft_size; i++)
                if (fft->output_stream->writeBuf[i] > max)
                    max = fft->output_stream->writeBuf[i];

            waterfall_plot->scale_min = fft_plot->scale_min = fft_plot->scale_min * 0.99 + min * 0.01;
            waterfall_plot->scale_max = fft_plot->scale_max = fft_plot->scale_max * 0.99 + max * 0.01;
            */
            waterfall_plot->draw({float(recorder_size.x * 0.80 - 4), float(recorder_size.y * 0.7) * 4});
        }
        ImGui::EndChild();
        ImGui::EndGroup();
    }
};