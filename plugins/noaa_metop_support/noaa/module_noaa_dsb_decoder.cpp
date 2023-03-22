#include "module_noaa_dsb_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>
#include "dsb.h"

// Return filesize
size_t getFilesize(std::string filepath);

#define BUFFER_SIZE 8192

namespace noaa
{
    NOAADSBDecoderModule::NOAADSBDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                  constellation(1.0, 0.15, demod_constellation_size)
    {
        def = std::make_shared<DSB_Deframer>(DSB_FRAME_SIZE * 8, 0);
        soft_buffer = new int8_t[BUFFER_SIZE];
    }

    std::vector<ModuleDataType> NOAADSBDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> NOAADSBDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    NOAADSBDecoderModule::~NOAADSBDecoderModule()
    {
        delete[] soft_buffer;
    }

    void NOAADSBDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".tip", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".tip");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".tip");

        uint8_t *output_pkts = new uint8_t[BUFFER_SIZE * 2];

        time_t lastTime = 0;
        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)soft_buffer, BUFFER_SIZE);
            else
                input_fifo->read((uint8_t *)soft_buffer, BUFFER_SIZE);

            int framen = def->work(soft_buffer, BUFFER_SIZE, output_pkts);

            // Count frames
            frame_count += framen;

            // Write to file
            if (framen > 0)
            {
                for (int i = 0; i < framen; i++)
                {
                    data_out.write((char *)&output_pkts[i * DSB_FRAME_SIZE], DSB_FRAME_SIZE);
                }
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string deframer_state = def->getState() == 0 ? "NOSYNC" : (def->getState() == 2 || def->getState() == 6 ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Deframer : " + deframer_state + ", Frames : " + std::to_string(frame_count));
            }
        }

        delete[] output_pkts;

        logger->info("Decoding finished");

        data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void NOAADSBDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("NOAA DSB Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        constellation.pushSofttAndGaussian(soft_buffer, 127, BUFFER_SIZE);
        constellation.draw();
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (def->getState() == def->STATE_NOSYNC)
                    ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                else if (def->getState() == def->STATE_SYNCING)
                    ImGui::TextColored(IMCOLOR_SYNCING, "SYNCING");
                else
                    ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");

                ImGui::Text("Frames : ");

                ImGui::SameLine();

                ImGui::TextColored(ImColor::HSV(113.0 / 360.0, 1, 1, 1.0), UITO_C_STR(frame_count));
            }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string NOAADSBDecoderModule::getID()
    {
        return "noaa_dsb_decoder";
    }

    std::vector<std::string> NOAADSBDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> NOAADSBDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<NOAADSBDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace noaa
