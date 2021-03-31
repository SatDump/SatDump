#include "module_oqpsk_demod.h"
#include <dsp/fir_gen.h>
#include "logger.h"
#include "imgui/imgui.h"

// Return filesize
size_t getFilesize(std::string filepath);

OQPSKDemodModule::OQPSKDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                          d_agc_rate(std::stof(parameters["agc_rate"])),
                                                                                                                                          d_samplerate(std::stoi(parameters["samplerate"])),
                                                                                                                                          d_symbolrate(std::stoi(parameters["symbolrate"])),
                                                                                                                                          d_rrc_alpha(std::stof(parameters["rrc_alpha"])),
                                                                                                                                          d_rrc_taps(std::stoi(parameters["rrc_taps"])),
                                                                                                                                          d_loop_bw(std::stof(parameters["costas_bw"])),
                                                                                                                                          d_dc_block(std::stoi(parameters["dc_block"])),
                                                                                                                                          d_buffer_size(std::stoi(parameters["buffer_size"])),
                                                                                                                                          d_clock_gain_omega(std::stof(parameters["clock_gain_omega"])),
                                                                                                                                          d_clock_mu(std::stof(parameters["clock_mu"])),
                                                                                                                                          d_clock_gain_mu(std::stof(parameters["clock_gain_mu"])),
                                                                                                                                          d_clock_omega_relative_limit(std::stof(parameters["clock_omega_relative_limit"])),
                                                                                                                                          d_const_scale(std::stof(parameters["constellation_scale"]))
{
    // Init DSP blocks
    file_source = std::make_shared<dsp::FileSourceBlock>(d_input_file, dsp::BasebandTypeFromString(parameters["baseband_format"]), d_buffer_size);
    if (d_dc_block)
        dcb = std::make_shared<dsp::DCBlockerBlock>(file_source->output_stream, 1024, true);
    agc = std::make_shared<dsp::AGCBlock>(d_dc_block ? dcb->output_stream : file_source->output_stream, d_agc_rate, 1.0f, 1.0f, 65536);
    rrc = std::make_shared<dsp::CCFIRBlock>(agc->output_stream, 1, libdsp::firgen::root_raised_cosine(1, d_samplerate, d_symbolrate, d_rrc_alpha, d_rrc_taps));
    pll = std::make_shared<dsp::CostasLoopBlock>(rrc->output_stream, d_loop_bw, 4);
    del = std::make_shared<dsp::DelayOneImagBlock>(pll->output_stream);
    rec = std::make_shared<dsp::CCMMClockRecoveryBlock>(del->output_stream, (float)d_samplerate / (float)d_symbolrate, d_clock_gain_omega, d_clock_mu, d_clock_gain_mu, d_clock_omega_relative_limit);

    // Buffers
    sym_buffer = new int8_t[d_buffer_size * 2];
}

OQPSKDemodModule::~OQPSKDemodModule()
{
    delete[] sym_buffer;
}

void OQPSKDemodModule::process()
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
    file_source->start();
    if (d_dc_block)
        dcb->start();
    agc->start();
    rrc->start();
    pll->start();
    del->start();
    rec->start();

    int dat_size = 0;
    while (/*input_data_type == DATA_STREAM ? input_active.load() : */ !file_source->eof())
    {
        dat_size = rec->output_stream->read(); //->read(rec_buffer2, d_buffer_size);

        if (dat_size <= 0)
            continue;

        for (int i = 0; i < dat_size; i++)
        {
            sym_buffer[i * 2] = clamp(rec->output_stream->readBuf[i].imag() * d_const_scale);
            sym_buffer[i * 2 + 1] = clamp(rec->output_stream->readBuf[i].real() * d_const_scale);
        }

        rec->output_stream->flush();

        data_out.write((char *)sym_buffer, dat_size * 2);

        progress = file_source->getPosition();
        if (time(NULL) % 10 == 0 && lastTime != time(NULL))
        {
            lastTime = time(NULL);
            logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
        }
    }

    logger->info("Demodulation finished");

    // Stop
    file_source->stop();
    if (d_dc_block)
        dcb->stop();
    agc->stop();
    rrc->stop();
    pll->stop();
    del->stop();
    rec->stop();

    data_out.close();
}

void OQPSKDemodModule::drawUI(bool window)
{
    ImGui::Begin("OQPSK Demodulator", NULL, window ? NULL : NOWINDOW_FLAGS );

    // Constellation
    {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                 ImVec2(ImGui::GetCursorScreenPos().x + 200, ImGui::GetCursorScreenPos().y + 200),
                                 ImColor::HSV(0, 0, 0));

        for (int i = 0; i < 2048; i++)
        {
            draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 + (sym_buffer[i * 2 + 0] / 127.0) * 100) % 200,
                                              ImGui::GetCursorScreenPos().y + (int)(100 + (sym_buffer[i * 2 + 1] / 127.0) * 100) % 200),
                                       2,
                                       ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
        }

        ImGui::Dummy(ImVec2(200 + 3, 200 + 3));
    }

    ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20));

    ImGui::End();
}

std::string OQPSKDemodModule::getID()
{
    return "oqpsk_demod";
}

std::vector<std::string> OQPSKDemodModule::getParameters()
{
    return {"samplerate", "symbolrate", "agc_rate", "rrc_alpha", "rrc_taps", "costas_bw", "iq_invert", "buffer_size", "clock_gain_omega", "clock_mu", "clock_gain_mu", "clock_omega_relative_limit", "constellation_scale", "baseband_format"};
}

std::shared_ptr<ProcessingModule> OQPSKDemodModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
{
    return std::make_shared<OQPSKDemodModule>(input_file, output_file_hint, parameters);
}