#include "live_run.h"
#include "imgui/imgui.h"
#include "sdr/airspy.h"
#include "global.h"
#include <fftw3.h>
#include <cstring>
#include "logger.h"
#include <volk/volk_alloc.hh>
#include "common/dsp/file_source.h"
#include "settings.h"
#include "live_pipeline.h"

#ifdef BUILD_LIVE
#define FFT_BUFFER_SIZE 8192
bool live_processing = false;
float fft_buffer[FFT_BUFFER_SIZE];
extern std::shared_ptr<SDRDevice> radio;
float scale = 1.0f;
std::shared_ptr<dsp::stream<std::complex<float>>> moduleStream;
extern std::shared_ptr<LivePipeline> live_pipeline;

bool process_fft = false;
std::mutex fft_run;

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

    //dsp::FileSourceBlock files("/home/alan/Downloads/10-36-51_1701300000Hz.wav", dsp::BasebandType::INTEGER_16, 8192);
    //files.start();

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
        }

        std::memcpy(sample_buffer, sample_buffer_vec.data(), FFT_BUFFER_SIZE * sizeof(std::complex<float>));
        std::memcpy(moduleStream->writeBuf, sample_buffer, FFT_BUFFER_SIZE * sizeof(std::complex<float>));
        moduleStream->swap(FFT_BUFFER_SIZE);
        sample_buffer_vec.erase(sample_buffer_vec.begin(), sample_buffer_vec.begin() + FFT_BUFFER_SIZE);

        if (y % runs_to_wait == 0)
        {
            fftwf_execute(p);

            for (int i = 0; i < FFT_BUFFER_SIZE / 2; i++)
            {
                float a = buffer_fft_out[i].real();
                float b = buffer_fft_out[i].imag();
                float c = sqrt(a * a + b * b);

                float x = buffer_fft_out[FFT_BUFFER_SIZE / 2 + i].real();
                float y = buffer_fft_out[FFT_BUFFER_SIZE / 2 + i].imag();
                float z = sqrt(x * x + y * y);

                fftb[i] = z * 4.0f * scale;
                fftb[FFT_BUFFER_SIZE / 2 + i] = c * 4.0f * scale;
            }

            for (int i = 0; i < FFT_BUFFER_SIZE; i++)
            {
                fft_buffer[i] = (fftb[i] * 1 + fft_buffer[i] * 9) / 10;
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

bool showUI = false;

void startRealLive()
{
    if (settings.count("fft_scale") > 0)
        scale = settings["fft_scale"].get<float>();

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
    settings["fft_scale"] = scale;
    settings["sdr"][radio->getID()] = radio->getParameters();
    saveSettings();

    logger->info("Destroying objects");
    radio.reset();
    live_pipeline.reset();
    moduleStream.reset();
    logger->info("Live exited!");

    initLive();

    live_processing = false;
}

void renderLive()
{

    // Safety
    if (showUI)
    {
        radio->drawUI();

        //ImGui::SetNextWindowPos({0, 0});
        live_pipeline->drawUIs();
        //ImGui::Text("LIVE");

        ImGui::Begin("Input FFT", NULL);
        ImGui::PlotLines("", fft_buffer, IM_ARRAYSIZE(fft_buffer), 0, 0, 0, 100, {std::max<float>(ImGui::GetWindowWidth() - 3, 200), std::max<float>(ImGui::GetWindowHeight() - 64, 100)});
        ImGui::SliderFloat("Scale", &scale, 0, 22);
        ImGui::SameLine();
        if (ImGui::Button("Stop"))
        {
            showUI = false;
            processThreadPool.push([=](int)
                                   { stopRealLive(); });
        }
        ImGui::End();
    }
}
#endif