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

#define FFT_SIZE (8192 * 1)
#define WATERFALL_RESOLUTION 1000

namespace recorder
{
    extern std::shared_ptr<SDRDevice> radio;
    extern int sample_format;
    float fft_buffer[FFT_SIZE];
    widgets::FFTPlot fftPlotWidget(fft_buffer, FFT_SIZE, 0, 1000, 15);
    bool recording = false;
    long long int recordedSize = 0;
    //long long int compressedSamples = 0;
    float scale = 40;
    std::ofstream data_out;
    std::mutex data_mutex;
    //bool enable_compression = false;

    uint32_t waterfallID;
    uint32_t *waterfall;
    uint32_t *waterfallPallet;

    bool shouldRun = false;
    std::mutex dspMutex, fftMutex;
    void doDSP(int);
    void doFFT(int);

    void initRecorder()
    {
        if (settings.count("recorder_scale") > 0)
            scale = settings["recorder_scale"].get<float>();

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

        processThreadPool.push(doDSP);
        processThreadPool.push(doFFT);
    }

    void exitRecorder()
    {
        volk_free(waterfall);
        delete[] waterfallPallet;

        settings["recorder_scale"] = scale;
        settings["recorder_sdr"][radio->getID()] = radio->getParameters();
        saveSettings();

        radio->stop();
        radio.reset();

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
                        data_out.close();
                        data_mutex.unlock();
                    }

                    fftMutex.lock();
                    fftMutex.unlock();

                    dspMutex.lock();
                    dspMutex.unlock();

                    exitRecorder();

                    ImGui::End();
                    return;
                }

                ImGui::SameLine();

                ImGui::SetNextItemWidth(ImGui::GetWindowWidth() / 3);
                ImGui::SliderFloat("Scale", &scale, 0, 100);

                ImGui::SameLine();
                if (recording)
                {
                    if (ImGui::Button("Stop Recording"))
                    {
                        recording = false;
                        data_mutex.lock();
                        data_out.close();
                        data_mutex.unlock();
                    }
                    ImGui::SameLine();
                    std::string datasize = (recordedSize > 1e9 ? to_string_with_precision<float>(recordedSize / 1e9, 2) + " GB" : to_string_with_precision<float>(recordedSize / 1e6, 2) + " MB");
                    ImGui::Text("Status : RECORDING, Size : %s", datasize.c_str());
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

                        std::string formatstr = "";
                        if (sample_format == 0)
                            formatstr = "i8";
                        else if (sample_format == 1)
                            formatstr = "i16";
                        else
                            formatstr = "f32";

                        std::string filename = default_recorder_output_folder + "/" + timestamp + "_" + std::to_string((long)radio->getSamplerate()) + "SPS_" +
                                               std::to_string((long)radio->getFrequency()) + "Hz." + formatstr;
                        //(enable_compression ? ".zst" : "");

                        logger->info("Recording to " + filename);

                        data_mutex.lock();
                        data_out = std::ofstream(filename, std::ios::binary);
                        data_mutex.unlock();

                        recordedSize = 0;

                        //if (enable_compression)
                        //    compressedSamples = 0;

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

    dsp::RingBuffer<std::complex<float>> circBuffer;

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
            int cnt = radio->output_stream->read();

            if (recording)
            {
                // Should probably add an AGC here...
                // Also maybe some buffering but as of now it's been doing OK.
                for (int i = 0; i < cnt; i++)
                {
                    // Clamp samples
                    radio->output_stream->readBuf[i] = std::complex<float>(clampF(radio->output_stream->readBuf[i].real()),
                                                                           clampF(radio->output_stream->readBuf[i].imag()));
                }

                // This is faster than a case
                if (sample_format == 0)
                {
                    volk_32f_s32f_convert_8i(converted_buffer_i8, (float *)radio->output_stream->readBuf, 127, cnt * 2); // Scale to 8-bits
                    data_out.write((char *)converted_buffer_i8, cnt * 2 * sizeof(uint8_t));
                    recordedSize += cnt * 2 * sizeof(uint8_t);
                }
                else if (sample_format == 1)
                {
                    volk_32f_s32f_convert_16i(converted_buffer_i16, (float *)radio->output_stream->readBuf, 65535, cnt * 2); // Scale to 16-bits
                    data_out.write((char *)converted_buffer_i16, cnt * 2 * sizeof(uint16_t));
                    recordedSize += cnt * 2 * sizeof(uint16_t);
                }
                else
                {
                    data_out.write((char *)radio->output_stream->readBuf, cnt * 2 * sizeof(float));
                    recordedSize += cnt * 2 * sizeof(float);
                }

                // Write them
                //if (!enable_compression)
                //{
                //data_out.write((char *)converted_buffer, cnt * 2);
                //}
                //else
                //{
                //    int ccnt = compressor.work((uint8_t *)converted_buffer, cnt * 2, compressed_buffer);
                //    data_out.write((char *)compressed_buffer, ccnt);
                //    compressedSamples += ccnt;
                //}
            }

            // Write to FFT FIFO
            if (circBuffer.getWritable(false) >= cnt)
                circBuffer.write(radio->output_stream->readBuf, cnt);

            radio->output_stream->flush();
        }

        if (sample_format == 0)
            delete[] converted_buffer_i8;
        if (sample_format == 1)
            delete[] converted_buffer_i16;
        //delete[] compressed_buffer;
        dspMutex.unlock();
    }

    void doFFT(int)
    {
        fftMutex.lock();
#ifdef __ANDROID__
        int refresh_per_second = 60; // We can assume FFTW will be slower.
#else
        int refresh_per_second = 60 * 2;
#endif
        int runs_per_second = radio->getSamplerate() / FFT_SIZE;
        int runs_to_wait = runs_per_second / refresh_per_second;
        //int run_wait = 1000.0f / (runs_per_second / runs_to_wait);
        int y = 0, z = 0;

        //logger->info(refresh_per_second);
        //logger->info(refresh_per_second);
        //logger->info(runs_per_second);
        //logger->info(runs_to_wait);

        float *fftb = (float *)volk_malloc(FFT_SIZE * sizeof(float), volk_get_alignment());
        std::complex<float> *sample_buffer = (std::complex<float> *)volk_malloc(FFT_SIZE * sizeof(std::complex<float>), volk_get_alignment());
        std::complex<float> *buffer_fft_out = (std::complex<float> *)volk_malloc(FFT_SIZE * sizeof(std::complex<float>), volk_get_alignment());

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
                    fft_buffer[i] = (std::max<float>(0, fftb[pos]) + fft_buffer[i] * 9) / 10;
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
    }
};
#endif