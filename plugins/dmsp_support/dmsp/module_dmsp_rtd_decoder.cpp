#include "module_dmsp_rtd_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>
#include "common/utils.h"

#define BUFFER_SIZE 8192
#define RTD_FRAME_SIZE 19

namespace dmsp
{
    DMSPRTDDecoderModule::DMSPRTDDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                  constellation(1.0, 0.15, demod_constellation_size)
    {
        def = std::make_shared<DMSP_Deframer>(150, 2);
        soft_buffer = new int8_t[BUFFER_SIZE];
        soft_bits = new uint8_t[BUFFER_SIZE];
        output_frames = new uint8_t[BUFFER_SIZE];
    }

    std::vector<ModuleDataType> DMSPRTDDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> DMSPRTDDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    DMSPRTDDecoderModule::~DMSPRTDDecoderModule()
    {
        delete[] soft_buffer;
        delete[] soft_bits;
        delete[] output_frames;
    }

    void DMSPRTDDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".frm", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".frm");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".frm");

        time_t lastTime = 0;
        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)soft_buffer, BUFFER_SIZE);
            else
                input_fifo->read((uint8_t *)soft_buffer, BUFFER_SIZE);

            for (int i = 0; i < BUFFER_SIZE; i++)
                soft_bits[i] = soft_buffer[i] > 0;

            int nframes = def->work(soft_bits, BUFFER_SIZE, output_frames);

            // Count frames
            // frame_count += nframes;

            // Write to file
            if (nframes > 0)
                data_out.write((char *)output_frames, nframes * RTD_FRAME_SIZE);

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string deframer_state = def->getState() == def->STATE_NOSYNC ? "NOSYNC" : (def->getState() == def->STATE_SYNCING ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%, Deframer : " + deframer_state);
            }
        }

        logger->info("Decoding finished");

        data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void DMSPRTDDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("DMSP RTD Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

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

                // ImGui::Text("Frames : ");

                // ImGui::SameLine();

                // ImGui::TextColored(ImColor::HSV(113.0 / 360.0, 1, 1, 1.0), UITO_C_STR(frame_count));
            }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string DMSPRTDDecoderModule::getID()
    {
        return "dmsp_rtd_decoder";
    }

    std::vector<std::string> DMSPRTDDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> DMSPRTDDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<DMSPRTDDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace noaa
