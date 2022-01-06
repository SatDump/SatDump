#include "module_cryosat_dump_decoder.h"
#include "logger.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "imgui/imgui.h"
#include "common/codings/differential/generic.h"
#include "common/codings/rotation.h"
#include "modules/cfosat/diff.h"
#include "common/dsp/constellation.h"

#define BUFFER_SIZE (1024 * 8 * 8)

// Returns the asked bit!
template <typename T>
inline bool getBit(T data, int bit)
{
    return (data >> bit) & 1;
}

// Return filesize
size_t getFilesize(std::string filepath);

namespace cryosat
{
    CRYOSATDumpDecoderModule::CRYOSATDumpDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        buffer = new uint8_t[BUFFER_SIZE];
    }

    CRYOSATDumpDecoderModule::~CRYOSATDumpDecoderModule()
    {
        delete[] buffer;
    }

    void CRYOSATDumpDecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        data_in = std::ifstream(d_input_file, std::ios::binary);
        data_out = std::ofstream(d_output_file_hint + ".cadu", std::ios::binary);
        d_output_files.push_back(d_output_file_hint + ".cadu");

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".cadu");

        time_t lastTime = 0;

        reedsolomon::ReedSolomon rs(reedsolomon::RS239);

        //  Buffers
        uint8_t diff_buffer[BUFFER_SIZE / 2];
        uint8_t diff_out[BUFFER_SIZE / 2];
        uint8_t repacked_buffer[BUFFER_SIZE / 8];

        // Custom diff
        cfosat::CFOSATDiff diff;
        //diff::GenericDiff diff(4);

        dsp::constellation_t constellation(dsp::QPSK);

        while (!data_in.eof())
        {
            // Read buffer
            data_in.read((char *)buffer, BUFFER_SIZE);

            rotate_soft((int8_t *)buffer, BUFFER_SIZE, PHASE_0, true);

            for (int i = 0; i < BUFFER_SIZE / 2; i++)
                diff_buffer[i] = constellation.soft_demod((int8_t *)&buffer[i * 2]);

            // Perform differential decoding
            diff.work(diff_buffer, BUFFER_SIZE / 2, diff_out);

            for (int i = 0; i < BUFFER_SIZE / 8; i++)
            {
                repacked_buffer[i] = diff_out[i * 4] << 6 | diff_out[i * 4 + 1] << 4 | diff_out[i * 4 + 2] << 2 | diff_out[i * 4 + 3];
                repacked_buffer[i] ^= 0xFF;
            }

            // Deframe that! (Integrated derand)
            std::vector<std::array<uint8_t, ccsds::ccsds_1_0_jason::CADU_SIZE>> frameBuffer = deframer.work(repacked_buffer, (BUFFER_SIZE / 8));

            // If we found frames, write them out
            if (frameBuffer.size() > 0)
            {
                for (std::array<uint8_t, ccsds::ccsds_1_0_jason::CADU_SIZE> cadu : frameBuffer)
                {
                    // RS Correction
                    rs.decode_interlaved(&cadu[4], true, 5, errors);

                    if (errors[0] >= 0 && errors[1] >= 0 && errors[2] >= 0 && errors[3] >= 0 && errors[4] >= 0)
                        // Write it to our output file!
                        data_out.write((char *)&cadu, ccsds::ccsds_1_0_jason::CADU_SIZE);
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

    void CRYOSATDumpDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("CRYOSAT Dump Decoder", NULL, window ? NULL : NOWINDOW_FLAGS);

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
                for (int i = 0; i < 5; i++)
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

    std::string CRYOSATDumpDecoderModule::getID()
    {
        return "cryosat_dump_decoder";
    }

    std::vector<std::string> CRYOSATDumpDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> CRYOSATDumpDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<CRYOSATDumpDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace aqua