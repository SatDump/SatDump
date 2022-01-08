#include "recorder.h"

#ifdef BUILD_LIVE
#include "imgui/imgui.h"
#include "global.h"
#include "logger.h"
#include "sdr/sdr.h"
#include "settings.h"
#include "settingsui.h"
#include "main_ui.h"
#include "common/widgets/fft_plot.h"
#include "common/utils.h"
#include <fstream>
#include <fftw3.h>
#include "imgui/imgui_image.h"
#include "colormaps.h"
#include "resources.h"
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef BUILD_ZIQ
#include "common/ziq.h"
#endif
#include "common/dsp/rational_resampler.h"

#define FFT_SIZE (8192 * 1)
#define WATERFALL_RESOLUTION 1000

namespace recorder
{
    extern std::shared_ptr<SDRDevice> radio;
    extern int sample_format;
    extern int decimation;
    float fft_buffer[FFT_SIZE];
    widgets::FFTPlot fftPlotWidget(fft_buffer, FFT_SIZE, 0, 1000, 15);
    bool recording = false;
    long long int recordedSize = 0;
    float scale = 40, offset = 0;
    std::ofstream data_out;
    std::mutex data_mutex;

    uint32_t waterfallID;
    uint32_t *waterfall;
    uint32_t *waterfallPallet;

    bool shouldRun = false;
    std::mutex dspMutex, fftMutex;
    void doDSP(int);
    void doFFT(int);
    dsp::RingBuffer<complex_t> circBuffer;

    std::shared_ptr<dsp::stream<complex_t>> sampleInputStream;
    std::shared_ptr<dsp::CCRationalResamplerBlock> resampler;

#ifdef BUILD_ZIQ
    // ZIQ
    std::shared_ptr<ziq::ziq_writer> ziqWriter;
    bool enable_ziq = false;
    long long int compressedSamples = 0;
#endif

    void initRecorder()
    {
        if (settings.count("recorder_scale") > 0)
            scale = settings["recorder_scale"].get<float>();
        if (settings.count("recorder_offset") > 0)
            offset = settings["recorder_offset"].get<float>();

        waterfall = (uint32_t *)volk_malloc(FFT_SIZE * 2000 * sizeof(uint32_t), volk_get_alignment());
        waterfallPallet = new uint32_t[1000];

        std::fill(fft_buffer, &fft_buffer[FFT_SIZE], 10);
        std::fill(waterfall, &waterfall[FFT_SIZE * 2000], 0);

        // This is adepted from SDR++, for the palette handling, credits to Ryzerth
        {
            colormaps::Map map = colormaps::loadMap(resources::getResourcePath("waterfall/classic.json"));
            int colorCount = map.entryCount;

            for (int i = 0; i < WATERFALL_RESOLUTION; i++)
            {
                int lowerId = floorf(((float)i / (float)WATERFALL_RESOLUTION) * colorCount);
                int upperId = ceilf(((float)i / (float)WATERFALL_RESOLUTION) * colorCount);
                lowerId = std::clamp<int>(lowerId, 0, colorCount - 1);
                upperId = std::clamp<int>(upperId, 0, colorCount - 1);
                float ratio = (((float)i / (float)WATERFALL_RESOLUTION) * colorCount) - lowerId;
                float r = (map.map[(lowerId * 3) + 0] * (1.0 - ratio)) + (map.map[(upperId * 3) + 0] * (ratio));
                float g = (map.map[(lowerId * 3) + 1] * (1.0 - ratio)) + (map.map[(upperId * 3) + 1] * (ratio));
                float b = (map.map[(lowerId * 3) + 2] * (1.0 - ratio)) + (map.map[(upperId * 3) + 2] * (ratio));
                waterfallPallet[i] = ((uint32_t)255 << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r;
            }
        }

        waterfallID = makeImageTexture();

#ifdef _WIN32
        logger->info("Setting process priority to Realtime");
        SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
#endif

        shouldRun = true;

        if (decimation > 1)
        {
            resampler = std::make_shared<dsp::CCRationalResamplerBlock>(radio->output_stream, 1, decimation);
            sampleInputStream = resampler->output_stream;
            resampler->start();
        }
        else
        {
            sampleInputStream = radio->output_stream;
        }

        processThreadPool.push(doDSP);
        processThreadPool.push(doFFT);
    }

    void exitRecorder()
    {
        settings["recorder_scale"] = scale;
        settings["recorder_offset"] = offset;
        settings["recorder_sdr"][radio->getID()] = radio->getParameters();
        saveSettings();

        if (decimation > 1)
        {
            resampler->stop();
            radio->output_stream->stopWriter();
            radio->output_stream->stopReader();
        }

        sampleInputStream->stopWriter();
        sampleInputStream->stopReader();
        radio->stop();
        radio.reset();

        volk_free(waterfall);
        delete[] waterfallPallet;

        satdumpUiStatus = MAIN_MENU;
    }

    std::atomic<bool> waterfallWasUpdated = false;

    void renderRecorder(int wwidth, int wheight)
    {
        if (shouldRun)
        {
            ImGui::SetNextWindowPos({0, 0});
            ImGui::SetNextWindowSize({(float)wwidth, (float)wheight});
            ImGui::Begin("Baseband Recorder", NULL, NOWINDOW_FLAGS | ImGuiWindowFlags_NoTitleBar);
            {
                fftPlotWidget.scale_max = scale;
                fftPlotWidget.draw(ImVec2(ImGui::GetWindowWidth() - 16 * ui_scale, (ImGui::GetWindowHeight() / 4) * 1));

                if (waterfallWasUpdated)
                {
                    updateImageTexture(waterfallID, waterfall, FFT_SIZE, 2000);
                    waterfallWasUpdated = false;
                }
                ImGui::Image((void *)(intptr_t)waterfallID, {ImGui::GetWindowWidth() - 16, (ImGui::GetWindowHeight() / 4) * 3 - 46}, {0, 0}, {1, 0.2});

                if (ImGui::Button("Exit"))
                {
                    shouldRun = false;

                    if (recording)
                    {
                        recording = false;
                        data_mutex.lock();
#ifdef BUILD_ZIQ
                        if (enable_ziq)
                            ziqWriter.reset();
#endif
                        data_out.close();
                        data_mutex.unlock();
                    }

                    circBuffer.stopReader();
                    circBuffer.stopWriter();

                    fftMutex.lock();
                    fftMutex.unlock();

                    dspMutex.lock();
                    dspMutex.unlock();

                    logger->info("Stopped");

                    exitRecorder();

                    ImGui::End();
                    return;
                }

                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 4);
                ImGui::SliderFloat("Scale", &scale, 0, 100);

                ImGui::SameLine();
                ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 4);
                ImGui::SliderFloat("Offset", &offset, -50, 50);

                ImGui::SameLine();
                if (recording)
                {
                    if (ImGui::Button("Stop Recording"))
                    {
                        recording = false;
                        data_mutex.lock();
#ifdef BUILD_ZIQ
                        if (enable_ziq)
                            ziqWriter.reset();
#endif
                        data_out.close();
                        data_mutex.unlock();
                    }
                    ImGui::SameLine();

                    std::string datasize = (recordedSize > 1e9 ? to_string_with_precision<float>(recordedSize / 1e9, 2) + " GB" : to_string_with_precision<float>(recordedSize / 1e6, 2) + " MB");

#ifdef BUILD_ZIQ
                    if (enable_ziq)
                    {
                        std::string compressedsize = (compressedSamples > 1e9 ? to_string_with_precision<float>(compressedSamples / 1e9, 2) + " GB" : to_string_with_precision<float>(compressedSamples / 1e6, 2) + " MB");
                        ImGui::Text("Status : RECORDING, Size : %s, Compressed : %s", datasize.c_str(), compressedsize.c_str());
                    }
                    else
#endif
                    {
                        ImGui::Text("Status : RECORDING, Size : %s", datasize.c_str());
                    }
                }
                else
                {
                    if (ImGui::Button("Start Recording"))
                    {
                        const time_t timevalue = time(0);
                        std::tm *timeReadable = gmtime(&timevalue);
                        std::string timestamp =
                            (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                            (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + "-" +
                            (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));

#ifdef BUILD_ZIQ
                        ziq::ziq_cfg cfg;
                        enable_ziq = false;
#endif

                        std::string formatstr = "";
                        if (sample_format == 0)
                            formatstr = "i8";
                        else if (sample_format == 1)
                            formatstr = "i16";
                        else if (sample_format == 2)
                            formatstr = "f32";
#ifdef BUILD_ZIQ
                        else if (sample_format == 3)
                        {
                            formatstr = "ziq";
                            cfg.is_compressed = true;
                            cfg.bits_per_sample = 8;
                            cfg.samplerate = radio->getSamplerate() / decimation;
                            cfg.annotation = "";
                            enable_ziq = true;
                        }
                        else if (sample_format == 4)
                        {
                            formatstr = "ziq";
                            cfg.is_compressed = true;
                            cfg.bits_per_sample = 16;
                            cfg.samplerate = radio->getSamplerate();
                            cfg.annotation = "";
                            enable_ziq = true;
                        }
                        else if (sample_format == 5)
                        {
                            formatstr = "ziq";
                            cfg.is_compressed = true;
                            cfg.bits_per_sample = 32;
                            cfg.samplerate = radio->getSamplerate();
                            cfg.annotation = "";
                            enable_ziq = true;
                        }
#endif
                        else
                            logger->critical("Something went horribly wrong with sample format!");

                        std::string filename = default_recorder_output_folder + "/" + timestamp + "_" + std::to_string((long)radio->getSamplerate()) + "SPS_" +
                                               std::to_string((long)radio->getFrequency()) + "Hz." + formatstr;

                        logger->info("Recording to " + filename);

                        data_mutex.lock();
                        data_out = std::ofstream(filename, std::ios::binary);
#ifdef BUILD_ZIQ
                        if (enable_ziq)
                            ziqWriter = std::make_shared<ziq::ziq_writer>(cfg, data_out);
#endif
                        data_mutex.unlock();

                        recordedSize = 0;

#ifdef BUILD_ZIQ
                        if (enable_ziq)
                            compressedSamples = 0;
#endif

                        recording = true;
                    }
                    ImGui::SameLine();
                    ImGui::Text("Status : IDLE");
                }
            }
            ImGui::End();

            radio->drawUI();
        }
    }

    float clampF(float c)
    {
        if (c > 1.0f)
            c = 1.0f;
        else if (c < -1.0f)
            c = -1.0f;
        return c;
    }

    void doDSP(int)
    {
        dspMutex.lock();
        circBuffer.init(1e9);
        int8_t *converted_buffer_i8 = nullptr;
        int16_t *converted_buffer_i16 = nullptr;
        if (sample_format == 0)
            converted_buffer_i8 = new int8_t[100000000];
        if (sample_format == 1)
            converted_buffer_i16 = new int16_t[100000000];
        //uint8_t *compressed_buffer = new uint8_t[100000000];

        while (shouldRun)
        {
            int cnt = sampleInputStream->read();

            if (recording)
            {
                // Should probably add an AGC here...
                // Also maybe some buffering but as of now it's been doing OK.
                for (int i = 0; i < cnt; i++)
                {
                    // Clamp samples
                    sampleInputStream->readBuf[i] = complex_t(clampF(sampleInputStream->readBuf[i].real),
                                                              clampF(sampleInputStream->readBuf[i].imag));
                }

                // This is faster than a case
                if (sample_format == 0)
                {
                    volk_32f_s32f_convert_8i(converted_buffer_i8, (float *)sampleInputStream->readBuf, 127, cnt * 2); // Scale to 8-bits
                    data_out.write((char *)converted_buffer_i8, cnt * 2 * sizeof(uint8_t));
                    recordedSize += cnt * 2 * sizeof(uint8_t);
                }
                else if (sample_format == 1)
                {
                    volk_32f_s32f_convert_16i(converted_buffer_i16, (float *)sampleInputStream->readBuf, 65535, cnt * 2); // Scale to 16-bits
                    data_out.write((char *)converted_buffer_i16, cnt * 2 * sizeof(uint16_t));
                    recordedSize += cnt * 2 * sizeof(uint16_t);
                }
                else if (sample_format == 2)
                {
                    data_out.write((char *)sampleInputStream->readBuf, cnt * 2 * sizeof(float));
                    recordedSize += cnt * 2 * sizeof(float);
                }
#ifdef BUILD_ZIQ
                else if (enable_ziq)
                {
                    compressedSamples += ziqWriter->write(sampleInputStream->readBuf, cnt);
                    recordedSize += cnt * 2 * sizeof(uint8_t);
                }
#endif
            }

            // Write to FFT FIFO
            if (circBuffer.getWritable(false) >= cnt)
                circBuffer.write(sampleInputStream->readBuf, cnt);

            sampleInputStream->flush();
        }

        if (sample_format == 0)
            delete[] converted_buffer_i8;
        if (sample_format == 1)
            delete[] converted_buffer_i16;
        //delete[] compressed_buffer;
        dspMutex.unlock();
        logger->info("DSP Quit");
    }

    void doFFT(int)
    {
        fftMutex.lock();
#ifdef __ANDROID__
        int refresh_per_second = 60; // We can assume FFTW will be slower.
#else
        int refresh_per_second = 60 * 2;
#endif
        int runs_per_second = (radio->getSamplerate() / decimation) / FFT_SIZE;
        int runs_to_wait = runs_per_second / refresh_per_second;
        //int run_wait = 1000.0f / (runs_per_second / runs_to_wait);
        int y = 0, z = 0;

        //logger->info(refresh_per_second);
        //logger->info(refresh_per_second);
        //logger->info(runs_per_second);
        //logger->info(runs_to_wait);

        float *fftb = (float *)volk_malloc(FFT_SIZE * sizeof(float), volk_get_alignment());
        complex_t *sample_buffer = (complex_t *)volk_malloc(FFT_SIZE * sizeof(complex_t), volk_get_alignment());
        complex_t *buffer_fft_out = (complex_t *)volk_malloc(FFT_SIZE * sizeof(complex_t), volk_get_alignment());

        fftwf_plan p = fftwf_plan_dft_1d(FFT_SIZE, (fftwf_complex *)sample_buffer, (fftwf_complex *)buffer_fft_out, FFTW_FORWARD, FFTW_ESTIMATE);

        while (shouldRun)
        {
            int cnt = circBuffer.read(sample_buffer, FFT_SIZE);

            if (cnt <= 0)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(1000));
                continue;
            }

            if (runs_to_wait == 0 ? true : (y % runs_to_wait == 0))
            {
                fftwf_execute(p);

                volk_32fc_s32f_x2_power_spectral_density_32f(fftb, (lv_32fc_t *)buffer_fft_out, 1, 1, FFT_SIZE);

                for (int i = 0; i < FFT_SIZE; i++)
                {
                    int pos = i + (i > (FFT_SIZE / 2) ? -(FFT_SIZE / 2) : (FFT_SIZE / 2));
                    fft_buffer[i] = (std::max<float>(0, fftb[pos] + offset) + fft_buffer[i] * 9) / 10;
                    if (z % 4 == 0)
                        waterfall[i] = waterfallPallet[std::min<int>(1000, std::max<int>(0, (fft_buffer[i] / scale) * 1000.0f))];
                }

                if (z > 10000000)
                    z = 0;
                z++;

                if (z % 4 == 0)
                {
                    std::memmove(&waterfall[FFT_SIZE], &waterfall[0], FFT_SIZE * 2000 - FFT_SIZE);

                    if (!waterfallWasUpdated)
                        waterfallWasUpdated = true;
                }
            }

            if (y == 10000000)
                y = 0;
            y++;
        }

        volk_free(sample_buffer);
        volk_free(buffer_fft_out);
        fftMutex.unlock();
        logger->info("FFT Quit");
    }
};
#endif