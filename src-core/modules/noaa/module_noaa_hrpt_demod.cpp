#include "module_noaa_hrpt_demod.h"
#include "modules/common/dsp/lib/fir_gen.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>

// Return filesize
size_t getFilesize(std::string filepath);

namespace noaa
{
    NOAAHRPTDemodModule::NOAAHRPTDemodModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                                    d_samplerate(std::stoi(parameters["samplerate"])),
                                                                                                                                                    d_buffer_size(std::stoi(parameters["buffer_size"]))
    {
        // Buffers
        bits_buffer = new uint8_t[d_buffer_size * 10];
    }

    void NOAAHRPTDemodModule::init()
    {
        // Init DSP blocks
        if (input_data_type == DATA_FILE)
            file_source = std::make_shared<dsp::FileSourceBlock>(d_input_file, dsp::BasebandTypeFromString(d_parameters["baseband_format"]), d_buffer_size);
        agc = std::make_shared<dsp::AGCBlock>(input_data_type == DATA_DSP_STREAM ? input_stream : file_source->output_stream, 0.002e-3f, 1.0f, 0.5f / 32768.0f, 65536);
        pll = std::make_shared<dsp::BPSKCarrierPLLBlock>(agc->output_stream, 0.01f, powf(0.01f, 2) / 4.0f, (3.0f * M_PI * 100e3f) / (float)d_samplerate);
        rrc = std::make_shared<dsp::FFFIRBlock>(pll->output_stream, 1, dsp::firgen::root_raised_cosine(1, (float)d_samplerate / 2.0f, 665400.0f, 0.5f, 31));
        rec = std::make_shared<dsp::FFMMClockRecoveryBlock>(rrc->output_stream, ((float)d_samplerate / (float)665400) / 2.0f, powf(0.01, 2) / 4.0f, 0.5f, 0.01f, 100e-6f);
        def = std::make_shared<NOAADeframer>(std::stoi(d_parameters["deframer_thresold"]));
    }

    std::vector<ModuleDataType> NOAAHRPTDemodModule::getInputTypes()
    {
        return {DATA_FILE, DATA_DSP_STREAM};
    }

    std::vector<ModuleDataType> NOAAHRPTDemodModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    NOAAHRPTDemodModule::~NOAAHRPTDemodModule()
    {
        delete[] bits_buffer;
    }

    void NOAAHRPTDemodModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = file_source->getFilesize();
        else
            filesize = 0;

        //if (input_data_type == DATA_FILE)
        //    data_in = std::ifstream(d_input_file, std::ios::binary);

        data_out = std::ofstream(d_output_file_hint + ".raw16", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".raw16");

        logger->info("Using input baseband " + d_input_file);
        logger->info("Demodulating to " + d_output_file_hint + ".raw16");
        logger->info("Buffer size : " + std::to_string(d_buffer_size));

        time_t lastTime = 0;

        // Start
        if (input_data_type == DATA_FILE)
            file_source->start();
        agc->start();
        pll->start();
        rrc->start();
        rec->start();

        int dat_size = 0;
        while (input_data_type == DATA_FILE ? !file_source->eof() : input_active.load())
        {
            dat_size = rec->output_stream->read();

            if (dat_size <= 0)
                continue;

            volk_32f_binary_slicer_8i((int8_t *)bits_buffer, rec->output_stream->readBuf, dat_size);

            rec->output_stream->flush();

            std::vector<uint16_t> frames = def->work(bits_buffer, dat_size);

            // Count frames
            frame_count += frames.size();

            // Write to file
            if (frames.size() > 0)
                data_out.write((char *)&frames[0], frames.size() * sizeof(uint16_t));

            if (input_data_type == DATA_FILE)
                progress = file_source->getPosition();
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Frames : " + std::to_string(frame_count / 11090));
            }
        }

        logger->info("Demodulation finished");

        // Stop
        if (input_data_type == DATA_FILE)
            file_source->stop();
        agc->stop();
        pll->stop();
        rrc->stop();
        rec->stop();

        data_out.close();
    }

    void NOAAHRPTDemodModule::drawUI(bool window)
    {
        ImGui::Begin("NOAA HRPT Demodulator", NULL, window ? NULL : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        // Constellation
        {
            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                     ImVec2(ImGui::GetCursorScreenPos().x + 200 * ui_scale, ImGui::GetCursorScreenPos().y + 200 * ui_scale),
                                     ImColor::HSV(0, 0, 0));

            for (int i = 0; i < 2048; i++)
            {
                draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + rec->output_stream->readBuf[i] * 90 * ui_scale) % int(200 * ui_scale),
                                                  ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + rng.gasdev() * 15 * ui_scale) % int(200 * ui_scale)),
                                           2 * ui_scale,
                                           ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
            }

            ImGui::Dummy(ImVec2(200 * ui_scale + 3, 200 * ui_scale + 3));
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("Frames : ");

                ImGui::SameLine();

                ImGui::TextColored(ImColor::HSV(113.0 / 360.0, 1, 1, 1.0), UITO_C_STR(frame_count / 11090));
            }
        }
        ImGui::EndGroup();

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string NOAAHRPTDemodModule::getID()
    {
        return "noaa_hrpt_demod";
    }

    std::vector<std::string> NOAAHRPTDemodModule::getParameters()
    {
        return {"samplerate", "buffer_size", "baseband_format", "deframer_thresold"};
    }

    std::shared_ptr<ProcessingModule> NOAAHRPTDemodModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<NOAAHRPTDemodModule>(input_file, output_file_hint, parameters);
    }

    std::vector<uint8_t> NOAAHRPTDemodModule::getBytes(uint8_t *bits, int length)
    {
        std::vector<uint8_t> bytesToRet;
        for (int ii = 0; ii < length; ii++)
        {
            byteToWrite = (byteToWrite << 1) | bits[ii];
            inByteToWrite++;

            if (inByteToWrite == 8)
            {
                bytesToRet.push_back(byteToWrite);
                inByteToWrite = 0;
            }
        }

        return bytesToRet;
    }
} // namespace noaa