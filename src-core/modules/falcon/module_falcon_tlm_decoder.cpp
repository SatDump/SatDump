#include "module_falcon_tlm_decoder.h"
#include "logger.h"
#include "modules/common/sathelper/derandomizer.h"
#include "modules/common/sathelper/reedsolomon_239.h"
#include "imgui/imgui.h"

#define BUFFER_SIZE 8192 * 10

// Returns the asked bit!
template <typename T>
inline bool getBit(T data, int bit)
{
    return (data >> bit) & 1;
}

// Return filesize
size_t getFilesize(std::string filepath);

namespace falcon
{
    FalconTLMDecoderModule::FalconTLMDecoderModule(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        buffer = new int8_t[BUFFER_SIZE];
    }

    FalconTLMDecoderModule::~FalconTLMDecoderModule()
    {
        delete[] buffer;
    }

    void FalconTLMDecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".cadu");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        sathelper::ReedSolomon239 reedSolomon;
        sathelper::Derandomizer derand;

        // Final buffer after decoding
        uint8_t finalBuffer[BUFFER_SIZE];

        // Bits => Bytes stuff
        uint8_t byteShifter = 0;
        int inByteShifter = 0;
        int byteShifted = 0;

        while (!data_in.eof())
        {
            // Read buffer
            data_in.read((char *)buffer, BUFFER_SIZE);

            // Group symbols into bytes now, I channel
            inByteShifter = 0;
            byteShifted = 0;

            for (int i = 0; i < BUFFER_SIZE; i++)
            {
                byteShifter = byteShifter << 1 | (buffer[i] > 0);
                inByteShifter++;

                if (inByteShifter == 8)
                {
                    finalBuffer[byteShifted++] = byteShifter;
                    inByteShifter = 0;
                }
            }

            // Deframe that! (Integrated derand)
            std::vector<std::array<uint8_t, ccsds::ccsds_1_0_proba::CADU_SIZE>> frameBuffer = deframer.work(finalBuffer, (BUFFER_SIZE / 8));

            // If we found frames, write them out
            if (frameBuffer.size() > 0)
            {
                for (std::array<uint8_t, ccsds::ccsds_1_0_proba::CADU_SIZE> cadu : frameBuffer)
                {
                    // RS Correction
                    for (int i = 0; i < 5; i++)
                    {
                        reedSolomon.deinterleave(&cadu[4], rsWorkBuffer, i, 5);
                        errors[i] = reedSolomon.decode_ccsds(rsWorkBuffer);
                        reedSolomon.interleave(rsWorkBuffer, &cadu[4], i, 5);
                    }

                    derand.work(&cadu[4], ccsds::ccsds_1_0_proba::CADU_SIZE - 4);

                    if (errors[0] > -1 && errors[1] > -1 && errors[2] > -1 && errors[3] > -1 && errors[4] > -1)
                        data_out.write((char *)&cadu, ccsds::ccsds_1_0_proba::CADU_SIZE);
                }
            }

            progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                std::string deframer_state = deframer.getState() == 0 ? "NOSYNC" : (deframer.getState() == 2 || deframer.getState() == 6 ? "SYNCING" : "SYNCED");
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%, Deframer : " + deframer_state);
            }
        }

        data_out.close();
        data_in.close();
    }

    const ImColor colorNosync = ImColor::HSV(0 / 360.0, 1, 1, 1.0);
    const ImColor colorSyncing = ImColor::HSV(39.0 / 360.0, 0.93, 1, 1.0);
    const ImColor colorSynced = ImColor::HSV(113.0 / 360.0, 1, 1, 1.0);

    void FalconTLMDecoderModule::drawUI()
    {
        ImGui::Begin("Falcon 9 TLM Decoder", NULL, NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        {
            // Constellation
            {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                         ImVec2(ImGui::GetCursorScreenPos().x + 200, ImGui::GetCursorScreenPos().y + 200),
                                         ImColor::HSV(0, 0, 0));

                for (int i = 0; i < 2048; i++)
                {
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 + (buffer[i] / 127.0) * 100) % 200,
                                                      ImGui::GetCursorScreenPos().y + (int)(100 + rng.gasdev() * 6) % 200),
                                               2,
                                               ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
                }

                ImGui::Dummy(ImVec2(200 + 3, 200 + 3));
            }
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Deframer", {200, 20});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (deframer.getState() == 0)
                    ImGui::TextColored(colorNosync, "NOSYNC");
                else if (deframer.getState() == 2 || deframer.getState() == 6)
                    ImGui::TextColored(colorSyncing, "SYNCING");
                else
                    ImGui::TextColored(colorSynced, "SYNCED");
            }

            ImGui::Spacing();

            ImGui::Button("Reed-Solomon", {200, 20});
            {
                ImGui::Text("RS    : ");
                for (int i = 0; i < 5; i++)
                {
                    ImGui::SameLine();

                    if (errors[i] == -1)
                        ImGui::TextColored(colorNosync, "%i ", i);
                    else if (errors[i] > 0)
                        ImGui::TextColored(colorSyncing, "%i ", i);
                    else
                        ImGui::TextColored(colorSynced, "%i ", i);
                }
            }
        }
        ImGui::EndGroup();

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20));

        ImGui::End();
    }

    std::string FalconTLMDecoderModule::getID()
    {
        return "falcon_tlm_decoder";
    }

    std::vector<std::string> FalconTLMDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> FalconTLMDecoderModule::getInstance(std::string input_file, std::string output_file_hint, std::map<std::string, std::string> parameters)
    {
        return std::make_shared<FalconTLMDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace falcon