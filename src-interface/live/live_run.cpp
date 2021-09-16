#include "live_run.h"

#ifdef BUILD_LIVE
#include "imgui/imgui.h"
#include "sdr/sdr.h"
#include "global.h"
#include <fftw3.h>
#include <cstring>
#include "logger.h"
#include <volk/volk_alloc.hh>
#include "common/dsp/file_source.h"
#include "settings.h"
#include "live_pipeline.h"
#include "global.h"
#include "processing.h"
#ifdef _WIN32
#include <windows.h>
#endif
#include "common/widgets/fft_plot.h"
#include "imgui/imgui_internal.h"
#include "main_ui.h"

namespace live
{
#define FFT_BUFFER_SIZE 8192

    bool live_processing = false;
    float fft_buffer[FFT_BUFFER_SIZE];
    extern std::shared_ptr<SDRDevice> radio;
    float scale_max = 100.0f, scale_offset = 0;
    std::shared_ptr<dsp::stream<std::complex<float>>> moduleStream;
    extern std::shared_ptr<LivePipeline> live_pipeline;

    bool process_fft = false;
    std::mutex fft_run;

    std::string input_level = "";
    std::string input_file = "";

    bool finishProcessing = false;

    void processFFT(int)
    {
        int refresh_per_second = 60 * 2;
        int runs_per_second = radio->getSamplerate() / FFT_BUFFER_SIZE;
        int runs_to_wait = runs_per_second / refresh_per_second;
        int i = 0, y = 0, cnt = 0;

        float fftb[FFT_BUFFER_SIZE];

        // std::complex<float> *sample_buffer = new std::complex<float>[8192 * 100];
        volk::vector<std::complex<float>> sample_buffer_vec;
        std::complex<float> sample_buffer[FFT_BUFFER_SIZE];
        std::complex<float> buffer_fft_out[FFT_BUFFER_SIZE];

        fftwf_plan p = fftwf_plan_dft_1d(FFT_BUFFER_SIZE, (fftwf_complex *)sample_buffer, (fftwf_complex *)buffer_fft_out, FFTW_FORWARD, FFTW_ESTIMATE);

        fft_run.lock();

        while (process_fft)
        {
            // This also reduces the buffer size for the demod
            if (sample_buffer_vec.size() < FFT_BUFFER_SIZE)
            {
                cnt = radio->output_stream->read();

                if (cnt > 0)
                {
                    sample_buffer_vec.insert(sample_buffer_vec.end(), radio->output_stream->readBuf, &radio->output_stream->readBuf[cnt]);
                    radio->output_stream->flush();
                }
                else
                {
                    continue;
                }

                if (sample_buffer_vec.size() < FFT_BUFFER_SIZE) // Still too small
                    continue;
            }

            std::memcpy(sample_buffer, sample_buffer_vec.data(), FFT_BUFFER_SIZE * sizeof(std::complex<float>));
            std::memcpy(moduleStream->writeBuf, sample_buffer, FFT_BUFFER_SIZE * sizeof(std::complex<float>));
            moduleStream->swap(FFT_BUFFER_SIZE);
            sample_buffer_vec.erase(sample_buffer_vec.begin(), sample_buffer_vec.begin() + FFT_BUFFER_SIZE);

            if (runs_to_wait == 0 ? true : (y % runs_to_wait == 0))
            {
                fftwf_execute(p);

                volk_32fc_s32f_x2_power_spectral_density_32f(fftb, (lv_32fc_t *)buffer_fft_out, 1, 1, FFT_BUFFER_SIZE);

                for (int i = 0; i < FFT_BUFFER_SIZE; i++)
                {
                    int pos = i + (i > (FFT_BUFFER_SIZE / 2) ? -(FFT_BUFFER_SIZE / 2) : (FFT_BUFFER_SIZE / 2));
                    fft_buffer[i] = (std::max<float>(0, fftb[pos] + scale_offset) + fft_buffer[i] * 9) / 10;
                }

                i++;

                if (i == 10000000)
                    i = 0;
            }

            if (y == 10000000)
                y = 0;
            y++;
        }

        logger->info("FFT exit");

        fft_run.unlock();
    }

    bool showUI = false, firstUIRun = false;

    widgets::FFTPlot fftPlotWidget(fft_buffer, FFT_BUFFER_SIZE, 0, 1000);

    void startRealLive()
    {
        if (settings.count("fft_scale") > 0)
            scale_max = settings["fft_scale"].get<float>();

        if (settings.count("fft_offset") > 0)
            scale_offset = settings["fft_offset"].get<float>();

        if (settings.count("live_finish_processing") > 0)
            finishProcessing = settings["live_finish_processing"].get<int>();

#ifdef _WIN32
        logger->info("Setting process priority to Realtime");
        SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
#endif

        // Start FFT
        process_fft = true;
        moduleStream = std::make_shared<dsp::stream<std::complex<float>>>();
        processThreadPool.push(processFFT);

        // Start pipeline
        live_pipeline->start(moduleStream, processThreadPool);

        firstUIRun = true;
        showUI = true;
    }

    void stopRealLive()
    {
        logger->info("Stop processing");
        live_pipeline->stop();

        logger->info("Stopping SDR");
        radio->stop();
        moduleStream->stopReader();
        moduleStream->stopWriter();

        logger->info("Stop FFT");
        process_fft = false;
        radio->output_stream->stopReader();
        radio->output_stream->stopWriter();
        logger->info("lock");
        fft_run.lock();
        fft_run.unlock();

        logger->info("Saving settings...");
        settings["fft_scale"] = scale_max;
        settings["fft_offset"] = scale_offset;
        settings["live_finish_processing"] = finishProcessing;
        settings["sdr"][radio->getID()] = radio->getParameters();
        saveSettings();

        bool doFinishProcessing = false;

        if (finishProcessing && live_pipeline->getOutputFiles().size() > 0)
        {
            input_file = live_pipeline->getOutputFiles()[0];
            doFinishProcessing = true;
        }

        logger->info("Destroying objects");
        radio.reset();
        live_pipeline.reset();
        moduleStream.reset();
        logger->info("Live exited!");

        if (doFinishProcessing)
        {
            std::vector<Pipeline>::iterator pipeline = std::find_if(pipelines.begin(),
                                                                    pipelines.end(),
                                                                    [](const Pipeline &e)
                                                                    {
                                                                        return e.name == downlink_pipeline;
                                                                    });
            int start_level = pipeline->live_cfg[pipeline->live_cfg.size() - 1].first;
            input_level = pipeline->steps[start_level].level_name;
            processThreadPool.push([&](int)
                                   { processing::process(downlink_pipeline, input_level, input_file, output_level, output_file, parameters); });
        }
        else
        {
            satdumpUiStatus = MAIN_MENU;
        }
    }

    void renderLive()
    {
        // Safety
        if (showUI)
        {
            if (firstUIRun)
            {
                firstUIRun = false;

                // Restore Window positions & sizes
                if (settings.contains("live_windows"))
                {
                    if (settings["live_windows"].contains(downlink_pipeline))
                    {
                        for (nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::json>> winJson : settings["live_windows"][downlink_pipeline].items())
                        {
                            // Find or create
                            ImGuiWindow *window = ImGui::FindWindowByName(winJson.key().c_str());
                            const bool window_just_created = (window == NULL);
                            if (window_just_created)
                            {
                                ImGui::Begin(winJson.key().c_str(), NULL);
                                ImGui::End();
                                window = ImGui::FindWindowByName(winJson.key().c_str());
                            }

                            nlohmann::json &win = winJson.value();
                            window->Size.x = win["size_x"].get<int>();
                            window->Size.y = win["size_y"].get<int>();
                            window->Pos.x = win["pos_x"].get<int>();
                            window->Pos.y = win["pos_y"].get<int>();
                        }
                    }
                }
            }

            radio->drawUI();

            //ImGui::SetNextWindowPos({0, 0});
            live_pipeline->drawUIs();
            //ImGui::Text("LIVE");

            ImGui::Begin("Input FFT", NULL);

            fftPlotWidget.scale_max = scale_max;
            fftPlotWidget.draw({std::max<float>(ImGui::GetWindowWidth() - 3 * ui_scale, 200 * ui_scale), std::max<float>(ImGui::GetWindowHeight() - 64 * ui_scale, 100 * ui_scale)});

            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 4);
            ImGui::SliderFloat("Scale", &scale_max, 0, 100);
            ImGui::SameLine();

            ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 4);
            ImGui::SliderFloat("Offset", &scale_offset, -50, 50);
            ImGui::SameLine();

            if (ImGui::Button("Stop"))
            {
                // Save windows positions & sizes
                for (ImGuiWindow *win : ImGui::GetCurrentContext()->Windows)
                {
                    if (win->Active)
                    {
                        nlohmann::json &winJson = settings["live_windows"][downlink_pipeline][win->Name];
                        winJson["size_x"] = win->Size.x;
                        winJson["size_y"] = win->Size.y;
                        winJson["pos_x"] = win->Pos.x;
                        winJson["pos_y"] = win->Pos.y;
                    }
                }

                showUI = false;
                processThreadPool.push([=](int)
                                       { stopRealLive(); });
            }
            ImGui::SameLine();
            ImGui::Checkbox("Finish processing", &finishProcessing);
            ImGui::End();
        }
    }
}
#endif