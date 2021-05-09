#include "module_qpsk_demod.h"
#include "modules/common/dsp/lib/fir_gen.h"
#include "logger.h"
#include "imgui/imgui.h"

// Return filesize
size_t getFilesize(std::string filepath)
{
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    std::size_t fileSize = file.tellg();
    file.close();
    return fileSize;
}

QPSKDemodModule::QPSKDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                        d_agc_rate(std::stof(parameters["agc_rate"])),
                                                                                                                                        d_samplerate(std::stoi(parameters["samplerate"])),
                                                                                                                                        d_symbolrate(std::stoi(parameters["symbolrate"])),
                                                                                                                                        d_rrc_alpha(std::stof(parameters["rrc_alpha"])),
                                                                                                                                        d_rrc_taps(std::stoi(parameters["rrc_taps"])),
                                                                                                                                        d_loop_bw(std::stof(parameters["costas_bw"])),
                                                                                                                                        d_buffer_size(std::stoi(parameters["buffer_size"])),
                                                                                                                                        d_dc_block(parameters.count("dc_block") > 0 ? std::stoi(parameters["dc_block"]) : 0),
                                                                                                                                        d_iq_swap(parameters.count("iq_swap") > 0 ? std::stoi(parameters["iq_swap"]) : 0)
{
    // Buffers
    sym_buffer = new int8_t[d_buffer_size * 2];
    snr = 0;
}

void QPSKDemodModule::init()
{
    // Init DSP blocks
    if (input_data_type == DATA_FILE)
        file_source = std::make_shared<dsp::FileSourceBlock>(d_input_file, dsp::BasebandTypeFromString(d_parameters["baseband_format"]), d_buffer_size);
    if (d_dc_block)
        dcb = std::make_shared<dsp::DCBlockerBlock>(input_data_type == DATA_DSP_STREAM ? input_stream : file_source->output_stream, 1024, true);
    agc = std::make_shared<dsp::AGCBlock>(d_dc_block ? dcb->output_stream : (input_data_type == DATA_DSP_STREAM ? input_stream : file_source->output_stream), d_agc_rate, 1.0f, 1.0f, 65536);
    rrc = std::make_shared<dsp::CCFIRBlock>(agc->output_stream, 1, dsp::firgen::root_raised_cosine(1, d_samplerate, d_symbolrate, d_rrc_alpha, d_rrc_taps));
    pll = std::make_shared<dsp::CostasLoopBlock>(rrc->output_stream, d_loop_bw, 4);
    rec = std::make_shared<dsp::CCMMClockRecoveryBlock>(pll->output_stream, (float)d_samplerate / (float)d_symbolrate, pow(8.7e-3, 2) / 4.0, 0.5f, 8.7e-3, 0.005f);
}

std::vector<ModuleDataType> QPSKDemodModule::getInputTypes()
{
    return {DATA_FILE, DATA_DSP_STREAM};
}

std::vector<ModuleDataType> QPSKDemodModule::getOutputTypes()
{
    return {DATA_FILE, DATA_STREAM};
}

QPSKDemodModule::~QPSKDemodModule()
{
    delete[] sym_buffer;
}

void QPSKDemodModule::process()
{
    if (input_data_type == DATA_FILE)
        filesize = file_source->getFilesize();
    else
        filesize = 0;

    if (output_data_type == DATA_FILE)
    {
        data_out = std::ofstream(d_output_file_hint + ".soft", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".soft");
    }

    logger->info("Using input baseband " + d_input_file);
    logger->info("Demodulating to " + d_output_file_hint + ".soft");
    logger->info("Buffer size : " + std::to_string(d_buffer_size));

    time_t lastTime = 0;

    // Start
    if (input_data_type == DATA_FILE)
        file_source->start();
    if (d_dc_block)
        dcb->start();
    agc->start();
    rrc->start();
    pll->start();
    rec->start();

    int dat_size = 0;
    while (input_data_type == DATA_FILE ? !file_source->eof() : input_active.load())
    {
        dat_size = rec->output_stream->read();

        if (dat_size <= 0)
            continue;

        // Estimate SNR, only on part of the samples to limit CPU usage
        snr_estimator.update(rec->output_stream->readBuf, dat_size / 100);
        snr = snr_estimator.snr();

        for (int i = 0; i < dat_size; i++)
        {
            sym_buffer[i * 2] = clamp(rec->output_stream->readBuf[i].imag() * 100);
            sym_buffer[i * 2 + 1] = clamp(rec->output_stream->readBuf[i].real() * 100);
        }

        rec->output_stream->flush();

        if (output_data_type == DATA_FILE)
            data_out.write((char *)sym_buffer, dat_size * 2);
        else
            output_fifo->write((uint8_t *)sym_buffer, dat_size * 2);

        if (input_data_type == DATA_FILE)
            progress = file_source->getPosition();
        if (time(NULL) % 10 == 0 && lastTime != time(NULL))
        {
            lastTime = time(NULL);
            logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, SNR : " + std::to_string(snr) + "dB");
        }
    }

    logger->info("Demodulation finished");

    // Stop
    if (input_data_type == DATA_FILE)
        file_source->stop();
    if (d_dc_block)
        dcb->stop();
    agc->stop();
    rrc->stop();
    pll->stop();
    rec->stop();

    if (output_data_type == DATA_FILE)
        data_out.close();
}

const ImColor colorNosync = ImColor::HSV(0 / 360.0, 1, 1, 1.0);
const ImColor colorSyncing = ImColor::HSV(39.0 / 360.0, 0.93, 1, 1.0);
const ImColor colorSynced = ImColor::HSV(113.0 / 360.0, 1, 1, 1.0);

void QPSKDemodModule::drawUI(bool window)
{
    ImGui::Begin("QPSK Demodulator", NULL, window ? NULL : NOWINDOW_FLAGS);

    ImGui::BeginGroup();
    // Constellation
    {
        ImDrawList *draw_list = ImGui::GetWindowDrawList();
        draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                 ImVec2(ImGui::GetCursorScreenPos().x + 200 * ui_scale, ImGui::GetCursorScreenPos().y + 200 * ui_scale),
                                 ImColor::HSV(0, 0, 0));

        for (int i = 0; i < 2048; i++)
        {
            draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (sym_buffer[i * 2 + 0] / 127.0) * 100 * ui_scale) % int(200 * ui_scale),
                                              ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + (sym_buffer[i * 2 + 1] / 127.0) * 100 * ui_scale) % int(200 * ui_scale)),
                                       2 * ui_scale,
                                       ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
        }

        ImGui::Dummy(ImVec2(200 * ui_scale + 3, 200 * ui_scale + 3));
    }
    ImGui::EndGroup();

    ImGui::SameLine();

    ImGui::BeginGroup();
    {
        ImGui::Button("Signal", {200 * ui_scale, 20 * ui_scale});
        {
            ImGui::Text("SNR (dB) : ");
            ImGui::SameLine();
            ImGui::TextColored(snr > 2 ? snr > 10 ? colorSynced : colorSyncing : colorNosync, UITO_C_STR(snr));

            std::memmove(&snr_history[0], &snr_history[1], (200 - 1) * sizeof(float));
            snr_history[200 - 1] = snr;

            ImGui::PlotLines("", snr_history, IM_ARRAYSIZE(snr_history), 0, "", 0.0f, 25.0f, ImVec2(200 * ui_scale, 50 * ui_scale));
        }
    }
    ImGui::EndGroup();

    ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

    ImGui::End();
}

std::string QPSKDemodModule::getID()
{
    return "qpsk_demod";
}

std::vector<std::string> QPSKDemodModule::getParameters()
{
    return {"samplerate", "symbolrate", "agc_rate", "rrc_alpha", "rrc_taps", "costas_bw", "iq_invert", "buffer_size", "dc_block", "baseband_format"};
}

std::shared_ptr<ProcessingModule> QPSKDemodModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
{
    return std::make_shared<QPSKDemodModule>(input_file, output_file_hint, parameters);
}