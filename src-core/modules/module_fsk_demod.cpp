#include "module_fsk_demod.h"
#include "common/dsp/lib/fir_gen.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>

// Return filesize
size_t getFilesize(std::string filepath);

FSKDemodModule::FSKDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                      d_samplerate(std::stoi(parameters["samplerate"])),
                                                                                                                                      d_symbolrate(std::stoi(parameters["symbolrate"])),
                                                                                                                                      d_buffer_size(std::stoi(parameters["buffer_size"])),
                                                                                                                                      d_lpf_cutoff(std::stoi(parameters["lpf_cutoff"])),
                                                                                                                                      d_lpf_transition_width(std::stoi(parameters["lpf_transition_width"]))
{
    // Buffers
    sym_buffer = new int8_t[d_buffer_size * 10];
}

void FSKDemodModule::init()
{
    // Init DSP blocks
    if (input_data_type == DATA_FILE)
        file_source = std::make_shared<dsp::FileSourceBlock>(d_input_file, dsp::BasebandTypeFromString(d_parameters["baseband_format"]), d_buffer_size);
    agc = std::make_shared<dsp::AGCBlock>(input_data_type == DATA_DSP_STREAM ? input_stream : file_source->output_stream, 0.0038e-3f, 1.0f, 0.5f / 32768.0f, 65536);
    lpf = std::make_shared<dsp::CCFIRBlock>(agc->output_stream, 1, dsp::firgen::low_pass(1, d_samplerate, d_lpf_cutoff, d_lpf_transition_width, dsp::fft::window::WIN_KAISER));
    qua = std::make_shared<dsp::QuadratureDemodBlock>(lpf->output_stream, 1.0f);
    rec = std::make_shared<dsp::FFMMClockRecoveryBlock>(qua->output_stream, (float)d_samplerate / (float)d_symbolrate, powf(0.01f, 2) / 4.0f, 0.5f, 0.01, 100e-6f);
}

std::vector<ModuleDataType> FSKDemodModule::getInputTypes()
{
    return {DATA_FILE, DATA_DSP_STREAM};
}

std::vector<ModuleDataType> FSKDemodModule::getOutputTypes()
{
    return {DATA_FILE};
}

FSKDemodModule::~FSKDemodModule()
{
    delete[] sym_buffer;
}

void FSKDemodModule::process()
{
    if (input_data_type == DATA_FILE)
        filesize = file_source->getFilesize();
    else
        filesize = 0;

    data_out = std::ofstream(d_output_file_hint + ".soft", std::ios::binary);
    d_output_files.push_back(d_output_file_hint + ".soft");

    logger->info("Using input baseband " + d_input_file);
    logger->info("Demodulating to " + d_output_file_hint + ".soft");
    logger->info("Buffer size : " + std::to_string(d_buffer_size));

    time_t lastTime = 0;

    // Start
    if (input_data_type == DATA_FILE)
        file_source->start();
    agc->start();
    lpf->start();
    qua->start();
    rec->start();

    int dat_size = 0;
    while (input_data_type == DATA_FILE ? !file_source->eof() : input_active.load())
    {
        dat_size = rec->output_stream->read();

        if (dat_size <= 0)
            continue;

        for (int i = 0; i < dat_size; i++)
        {
            sym_buffer[i] = clamp(rec->output_stream->readBuf[i] * 50);
        }

        rec->output_stream->flush();

        data_out.write((char *)sym_buffer, dat_size);

        if (input_data_type == DATA_FILE)
            progress = file_source->getPosition();
        if (time(NULL) % 10 == 0 && lastTime != time(NULL))
        {
            lastTime = time(NULL);
            logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
        }
    }

    logger->info("Demodulation finished");

    // Stop
    if (input_data_type == DATA_FILE)
        file_source->stop();
    agc->stop();
    lpf->stop();
    qua->stop();
    rec->stop();

    data_out.close();
}

void FSKDemodModule::drawUI(bool window)
{
    ImGui::Begin("FSK Demodulator", NULL, window ? NULL : NOWINDOW_FLAGS);

    // Constellation
    {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                 ImVec2(ImGui::GetCursorScreenPos().x + 200 * ui_scale, ImGui::GetCursorScreenPos().y + 200 * ui_scale),
                                 ImColor::HSV(0, 0, 0));

        for (int i = 0; i < 2048; i++)
        {
            draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + rec->output_stream->readBuf[i] * 45 * ui_scale) % int(200 * ui_scale),
                                              ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + rng.gasdev() * 15) % int(200 * ui_scale)),
                                       2 * ui_scale,
                                       ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
        }

        ImGui::Dummy(ImVec2(200 * ui_scale + 3, 200 * ui_scale + 3));
    }

    ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

    ImGui::End();
}

std::string FSKDemodModule::getID()
{
    return "fsk_demod";
}

std::vector<std::string> FSKDemodModule::getParameters()
{
    return {"samplerate", "buffer_size", "baseband_format", "symbolrate", "lpf_cutoff", "lpf_transition_width"};
}

std::shared_ptr<ProcessingModule> FSKDemodModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
{
    return std::make_shared<FSKDemodModule>(input_file, output_file_hint, parameters);
}