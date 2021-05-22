#include "live_run.h"
#include "imgui/imgui.h"
#include "sdr/airspy.h"
#include "global.h"
#include <fftw3.h>
#include <cstring>
#include "logger.h"
#include <volk/volk_alloc.hh>
#include "common/dsp/file_source.h"

#ifdef BUILD_LIVE
#define FFT_BUFFER_SIZE 8192
bool live_processing = false;
float fft_buffer[FFT_BUFFER_SIZE];
extern std::shared_ptr<SDRDevice> radio;
extern std::vector<std::shared_ptr<ProcessingModule>> liveModules;
float scale = 1.0f;
std::shared_ptr<dsp::stream<std::complex<float>>> moduleStream;

bool process_fft = false;
std::mutex fft_run;

void processFFT(int)
{
    int refresh_per_second = 20;
    int runs_to_wait = (radio->getSamplerate() / 8192) / (refresh_per_second * 3);
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

            for (int i = 0, iMax = FFT_BUFFER_SIZE / 2; i < iMax; i++)
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
#ifdef _WIN32
    logger->info("Setting process priority to Realtime");
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
#endif
    // 1
    process_fft = true;
    moduleStream = std::make_shared<dsp::stream<std::complex<float>>>();
    processThreadPool.push(processFFT);

    /* module1->input_stream = moduleStream;
    module1->setInputType(DATA_DSP_STREAM);
    module1->setOutputType(DATA_STREAM);
    module1->output_fifo = std::make_shared<RingBuffer<uint8_t>>(1000000);
    module1->init();
    module1->input_active = true;
    processThreadPool.push([=](int) { logger->info("Start processing...");
                                            module1->process(); });

    module2->input_fifo = module1->output_fifo;
    module2->setInputType(DATA_STREAM);
    module2->setOutputType(DATA_FILE);
    module2->init();
    module2->input_active = true;
    processThreadPool.push([=](int) { logger->info("Start processing...");
                                            module2->process(); });*/

    // Init first module in the chain, always a demod...
    liveModules[0]->input_stream = moduleStream;
    liveModules[0]->setInputType(DATA_DSP_STREAM);
    liveModules[0]->setOutputType(liveModules.size() > 1 ? DATA_STREAM : DATA_FILE);
    liveModules[0]->output_fifo = std::make_shared<dsp::RingBuffer<uint8_t>>(1000000);
    liveModules[0]->init();
    liveModules[0]->input_active = true;
    processThreadPool.push([=](int) { logger->info("Start processing...");
                                            liveModules[0]->process(); });

    // Init whatever's in the middle
    for (int i = 1; i < liveModules.size() - 1; i++)
    {
        liveModules[i]->input_fifo = liveModules[i - 1]->output_fifo;
        liveModules[i]->output_fifo = std::make_shared<dsp::RingBuffer<uint8_t>>(1000000);
        liveModules[i]->setInputType(DATA_STREAM);
        liveModules[i]->setOutputType(DATA_STREAM);
        liveModules[i]->init();
        liveModules[i]->input_active = true;
        processThreadPool.push([=](int) { logger->info("Start processing...");
                                            liveModules[i]->process(); });
    }

    // Init the last module
    if (liveModules.size() > 1)
    {
        int num = liveModules.size() - 1;
        liveModules[num]->input_fifo = liveModules[num - 1]->output_fifo;
        liveModules[num]->setInputType(DATA_STREAM);
        liveModules[num]->setOutputType(DATA_FILE);
        liveModules[num]->init();
        liveModules[num]->input_active = true;
        processThreadPool.push([=](int) { logger->info("Start processing...");
                                           liveModules[num]->process(); });
    }

    showUI = true;
}

void stopRealLive()
{
    logger->info("Stop processing");
    for (std::shared_ptr<ProcessingModule> mod : liveModules)
    {
        mod->input_active = false;

        if (mod->getInputType() == DATA_DSP_STREAM)
        {
            mod->input_stream->stopReader();
            mod->input_stream->stopWriter();
        }
        else if (mod->getInputType() == DATA_STREAM)
        {
            mod->input_fifo->stopReader();
            mod->input_fifo->stopWriter();
        }
        //logger->info("mod");
        mod->stop();
    }

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

    logger->info("Destroying objects");
    radio.reset();
    liveModules.clear();
    moduleStream.reset();
    logger->info("Live exited!");

    initLive();

    live_processing = false;
}

void renderLive()
{
    if (ImGui::Button("Stop"))
    {
        showUI = false;
        processThreadPool.push([=](int) { stopRealLive(); });
    }

    // Safety
    if (showUI)
    {
        radio->drawUI();

        //ImGui::SetNextWindowPos({0, 0});
        for (std::shared_ptr<ProcessingModule> mod : liveModules)
            mod->drawUI(true);
        //ImGui::Text("LIVE");

        ImGui::Begin("Input FFT", NULL);
        ImGui::PlotLines("", fft_buffer, IM_ARRAYSIZE(fft_buffer), 0, 0, 0, 100, {std::max<float>(ImGui::GetWindowWidth() - 3, 200), std::max<float>(ImGui::GetWindowHeight() - 64, 100)});
        ImGui::SliderFloat("Scale", &scale, 0, 22);
        ImGui::End();
    }
}
#endif