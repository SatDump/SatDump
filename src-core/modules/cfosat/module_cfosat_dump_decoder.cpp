#include "module_cfosat_dump_decoder.h"
#include "logger.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "diff.h"
#include "imgui/imgui.h"

#define BUFFER_SIZE (1024 * 8 * 8)

// Returns the asked bit!
template <typename T>
inline bool getBit(T data, int bit)
{
    return (data >> bit) & 1;
}

// Return filesize
size_t getFilesize(std::string filepath);

namespace cfosat
{
    CFOSATDumpDecoderModule::CFOSATDumpDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        buffer = new uint8_t[BUFFER_SIZE];
    }

    CFOSATDumpDecoderModule::~CFOSATDumpDecoderModule()
    {
        delete[] buffer;
    }

    void CFOSATDumpDecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".cadu");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        reedsolomon::ReedSolomon rs(reedsolomon::RS223);

        //  Buffers
        uint8_t diff_buffer[BUFFER_SIZE / 2];
        uint8_t diff_out[BUFFER_SIZE / 2];
        uint8_t repacked_buffer[BUFFER_SIZE / 8];

        // Custom diff
        CFOSATDiff diff;

        while (!data_in.eof())
        {
            // Read buffer
            data_in.read((char *)buffer, BUFFER_SIZE);

            // Demodulate QPSK... This is the crappy way but it works
            for (int i = 0; i < BUFFER_SIZE / 2; i++)
            {
                int8_t sample_i = clamp(*((int8_t *)&buffer[i * 2 + 1]));
                int8_t sample_q = clamp(*((int8_t *)&buffer[i * 2 + 0]));

                if (sample_i == -1 && sample_q == -1)
                    diff_buffer[i] = 0b00;
                else if (sample_i == 1 && sample_q == -1)
                    diff_buffer[i] = 0b01;
                else if (sample_i == 1 && sample_q == 1)
                    diff_buffer[i] = 0b11;
                else if (sample_i == -1 && sample_q == 1)
                    diff_buffer[i] = 0b10;
            }

            // Perform differential decoding
            diff.work(diff_buffer, BUFFER_SIZE / 2, diff_out);

            for (int i = 0; i < BUFFER_SIZE / 8; i++)
            {
                repacked_buffer[i] = diff_out[i * 4] << 6 | diff_out[i * 4 + 1] << 4 | diff_out[i * 4 + 2] << 2 | diff_out[i * 4 + 3];
            }

            // Deframe that! (Integrated derand)
            std::vector<std::array<uint8_t, ccsds::ccsds_1_0_1024::CADU_SIZE>> frameBuffer = deframer.work(repacked_buffer, (BUFFER_SIZE / 8));

            // If we found frames, write them out
            if (frameBuffer.size() > 0)
            {
                for (std::array<uint8_t, ccsds::ccsds_1_0_1024::CADU_SIZE> cadu : frameBuffer)
                {
                    // RS Correction
                    rs.decode_interlaved(&cadu[4], true, 4, errors);

                    // Write it to our output file!
                    data_out.write((char *)&cadu, ccsds::ccsds_1_0_1024::CADU_SIZE);
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

    void CFOSATDumpDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("CFOSAT-1 Dump Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        {
            // Constellation
            {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                draw_list->AddRectFilled(ImGui::GetCursorScreenPos(),
                                         ImVec2(ImGui::GetCursorScreenPos().x + 200 * ui_scale, ImGui::GetCursorScreenPos().y + 200 * ui_scale),
                                         ImColor::HSV(0, 0, 0));

                for (int i = 0; i < 2048; i++)
                {
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (((int8_t *)buffer)[i * 2 + 0] / 127.0) * 100 * ui_scale) % int(200 * ui_scale),
                                                      ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + (((int8_t *)buffer)[i * 2 + 1] / 127.0) * 100 * ui_scale) % int(200 * ui_scale)),
                                               2 * ui_scale,
                                               ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
                }

                ImGui::Dummy(ImVec2(200 * ui_scale + 3, 200 * ui_scale + 3));
            }
        }
        ImGui::EndGroup();

        ImGui::SameLine();

        ImGui::BeginGroup();
        {
            ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("State : ");

                ImGui::SameLine();

                if (deframer.getState() == 0)
                    ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                else if (deframer.getState() == 2 || deframer.getState() == 6)
                    ImGui::TextColored(IMCOLOR_SYNCING, "SYNCING");
                else
                    ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");
            }

            ImGui::Spacing();

            ImGui::Button("Reed-Solomon", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("RS    : ");
                for (int i = 0; i < 4; i++)
                {
                    ImGui::SameLine();

                    if (errors[i] == -1)
                        ImGui::TextColored(IMCOLOR_NOSYNC, "%i ", i);
                    else if (errors[i] > 0)
                        ImGui::TextColored(IMCOLOR_SYNCING, "%i ", i);
                    else
                        ImGui::TextColored(IMCOLOR_SYNCED, "%i ", i);
                }
            }
        }
        ImGui::EndGroup();

        ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string CFOSATDumpDecoderModule::getID()
    {
        return "cfosat_dump_decoder";
    }

    std::vector<std::string> CFOSATDumpDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> CFOSATDumpDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<CFOSATDumpDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace aqua