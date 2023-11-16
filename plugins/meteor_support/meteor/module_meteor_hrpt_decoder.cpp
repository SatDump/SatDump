#include "module_meteor_hrpt_decoder.h"
#include "logger.h"
#include "common/codings/manchester.h"
#include "imgui/imgui.h"

// Return filesize
uint64_t getFilesize(std::string filepath);

#define BUFFER_SIZE 8192

namespace meteor
{
    METEORHRPTDecoderModule::METEORHRPTDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                        constellation(1.0, 0.15, demod_constellation_size)
    {
        def = std::make_shared<CADUDeframer>();

        soft_buffer = new int8_t[BUFFER_SIZE];
    }

    std::vector<ModuleDataType> METEORHRPTDecoderModule::getInputTypes()
    {
        return {DATA_FILE, DATA_STREAM};
    }

    std::vector<ModuleDataType> METEORHRPTDecoderModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    METEORHRPTDecoderModule::~METEORHRPTDecoderModule()
    {
        delete[] soft_buffer;
    }

    void METEORHRPTDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".cadu");

        logger->info("Using input data " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        int frame_count = 0;

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)soft_buffer, BUFFER_SIZE);
            else
                input_fifo->read((uint8_t *)soft_buffer, BUFFER_SIZE);

            std::vector<std::array<uint8_t, CADU_SIZE>> frames = def->work(soft_buffer, BUFFER_SIZE);

            // Count frames
            frame_count += frames.size();

            // Write to file
            if (frames.size() > 0)
            {
                for (std::array<uint8_t, CADU_SIZE> frm : frames)
                {
                    data_out.write((char *)&frm[0], CADU_SIZE);
                }
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            module_stats["deframer_lock"] = def->getState() == 12;

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string deframer_state = def->getState() == 0 ? "NOSYNC" : (def->getState() == 2 || def->getState() == 6 ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%, Deframer : " + deframer_state + ", Frames : " + std::to_string(frame_count));
            }
        }

        logger->info("Decoding finished");

        data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    void METEORHRPTDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("METEOR HRPT Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        // Constellation
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

                if (def->getState() == 0)
                    ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                else if (def->getState() == 2 || def->getState() == 6)
                    ImGui::TextColored(IMCOLOR_SYNCING, "SYNCING");
                else
                    ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");
            }
        }
        ImGui::EndGroup();

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string METEORHRPTDecoderModule::getID()
    {
        return "meteor_hrpt_decoder";
    }

    std::vector<std::string> METEORHRPTDecoderModule::getParameters()
    {
        return {"samplerate", "buffer_size", "baseband_format"};
    }

    std::shared_ptr<ProcessingModule> METEORHRPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<METEORHRPTDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace meteor