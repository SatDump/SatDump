#include "module_meteor_hrpt_decoder.h"
#include "logger.h"
#include "modules/common/manchester.h"
#include "imgui/imgui.h"

// Return filesize
size_t getFilesize(std::string filepath);

#define BUFFER_SIZE 8192

namespace meteor
{
    METEORHRPTDecoderModule::METEORHRPTDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        def = std::make_shared<CADUDeframer>();

        read_buffer = new uint8_t[BUFFER_SIZE];
        manchester_buffer = new uint8_t[BUFFER_SIZE];
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
        delete[] manchester_buffer;
        delete[] read_buffer;
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
                data_in.read((char *)read_buffer, BUFFER_SIZE);
            else
                input_fifo->read((uint8_t *)read_buffer, BUFFER_SIZE);

            manchesterDecoder(read_buffer, BUFFER_SIZE, manchester_buffer);

            std::vector<std::array<uint8_t, CADU_SIZE>> frames = def->work(manchester_buffer, BUFFER_SIZE / 2);

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

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string deframer_state = def->getState() == 0 ? "NOSYNC" : (def->getState() == 2 || def->getState() == 6 ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Deframer : " + deframer_state + ", Frames : " + std::to_string(frame_count));
            }
        }

        logger->info("Decoding finished");

        data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
    }

    const ImColor colorNosync = ImColor::HSV(0 / 360.0, 1, 1, 1.0);
    const ImColor colorSyncing = ImColor::HSV(39.0 / 360.0, 0.93, 1, 1.0);
    const ImColor colorSynced = ImColor::HSV(113.0 / 360.0, 1, 1, 1.0);

    void METEORHRPTDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("METEOR HRPT Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        {
            ImGui::Button("Deframer", {200, 20});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (def->getState() == 0)
                    ImGui::TextColored(colorNosync, "NOSYNC");
                else if (def->getState() == 2 || def->getState() == 6)
                    ImGui::TextColored(colorSyncing, "SYNCING");
                else
                    ImGui::TextColored(colorSynced, "SYNCED");
            }
        }
        ImGui::EndGroup();

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20));

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

    std::shared_ptr<ProcessingModule> METEORHRPTDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<METEORHRPTDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace meteor