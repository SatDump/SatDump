#include "module_noaa_hrpt_decoder.h"
#include "common/dsp/firdes.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>

// Return filesize
size_t getFilesize(std::string filepath);

#define BUFSIZE 8192

namespace noaa
{
    NOAAHRPTDecoderModule::NOAAHRPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                    constellation(1.0, 0.15, demod_constellation_size)
    {
        // Buffers
        soft_buffer = new int8_t[BUFSIZE];
        def = std::make_shared<NOAADeframer>(d_parameters["deframer_thresold"].get<int>());
    }

    std::vector<ModuleDataType> NOAAHRPTDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> NOAAHRPTDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    NOAAHRPTDecoderModule::~NOAAHRPTDecoderModule()
    {
        delete[] soft_buffer;
    }

    void NOAAHRPTDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;

        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);

        data_out = std::ofstream(d_output_file_hint + ".raw16", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".raw16");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".raw16");

        time_t lastTime = 0;

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            if (input_data_type == DATA_FILE)
                data_in.read((char *)soft_buffer, BUFSIZE);
            else
                input_fifo->read((uint8_t *)soft_buffer, BUFSIZE);

            std::vector<uint16_t> frames = def->work(soft_buffer, BUFSIZE);

            // Count frames
            frame_count += frames.size();

            // Write to file
            if (frames.size() > 0)
                data_out.write((char *)&frames[0], frames.size() * sizeof(uint16_t));

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();
            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Frames : " + std::to_string(frame_count / 11090));
            }
        }

        logger->info("Demodulation finished");

        if (input_data_type == DATA_FILE)
            data_in.close();
        data_out.close();
    }

    void NOAAHRPTDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("NOAA HRPT Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

        // Constellation
        ImGui::BeginGroup();
        constellation.pushSofttAndGaussian(soft_buffer, 127, BUFSIZE);
        constellation.draw();
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

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string NOAAHRPTDecoderModule::getID()
    {
        return "noaa_hrpt_decoder";
    }

    std::vector<std::string> NOAAHRPTDecoderModule::getParameters()
    {
        return {"samplerate", "buffer_size", "baseband_format", "deframer_thresold"};
    }

    std::shared_ptr<ProcessingModule> NOAAHRPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<NOAAHRPTDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace noaa
