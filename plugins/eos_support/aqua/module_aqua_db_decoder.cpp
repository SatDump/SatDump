#include "module_aqua_db_decoder.h"
#include "common/codings/differential/nrzm.h"
#include "common/codings/randomization.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/dsp/demod/constellation.h"
#include "common/utils.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <cstdint>

#define BUFFER_SIZE (1024 * 8 * 8)
#define FRAME_SIZE 1024

namespace aqua
{
    AquaDBDecoderModule::AquaDBDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        buffer = new uint8_t[BUFFER_SIZE];
        deframer.STATE_SYNCING = 6;
        deframer.STATE_SYNCED = 10;
        fsfsm_file_ext = ".cadu";
    }

    AquaDBDecoderModule::~AquaDBDecoderModule() { delete[] buffer; }

    void AquaDBDecoderModule::process()
    {
        dsp::constellation_t constellation(dsp::QPSK);
        reedsolomon::ReedSolomon rs(reedsolomon::RS223);

        // I/Q Buffers
        uint8_t decodedBufI[BUFFER_SIZE / 2];
        uint8_t decodedBufQ[BUFFER_SIZE / 2];

        // Final buffer after decoding
        uint8_t finalBuffer[BUFFER_SIZE];

        // 2 NRZ-M Decoders
        diff::NRZMDiff diff1, diff2;

        uint8_t frame_buffer[FRAME_SIZE * 100];

        while (should_run())
        {
            // Read buffer
            read_data((uint8_t *)buffer, BUFFER_SIZE);

            // Demodulate QPSK Constellation
            for (int i = 0; i < BUFFER_SIZE / 2; i++)
            {
                uint8_t demod = constellation.soft_demod((int8_t *)&buffer[i * 2]);
                decodedBufI[i] = demod >> 1;
                decodedBufQ[i] = demod & 1;
            }

            diff1.decode_bits(decodedBufI, BUFFER_SIZE / 2);
            diff2.decode_bits(decodedBufQ, BUFFER_SIZE / 2);

            for (int i = 0; i < BUFFER_SIZE / 2; i++)
            {
                finalBuffer[i * 2 + 0] = decodedBufI[i];
                finalBuffer[i * 2 + 1] = decodedBufQ[i];
            }

            int frames = deframer.work(finalBuffer, BUFFER_SIZE, frame_buffer);

            for (int i = 0; i < frames; i++)
            {
                uint8_t *cadu = &frame_buffer[i * FRAME_SIZE];

                // Derand
                derand_ccsds(&cadu[4], FRAME_SIZE - 4);

                // RS Correction
                rs.decode_interlaved(&cadu[4], true, 4, errors);

                // Write it out
                write_data((uint8_t *)cadu, FRAME_SIZE);
            }
        }

        cleanup();
    }

    nlohmann::json AquaDBDecoderModule::getModuleStats()
    {
        auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
        std::string deframer_state = deframer.getState() == deframer.STATE_NOSYNC ? "NOSYNC" : (deframer.getState() == deframer.STATE_SYNCING ? "SYNCING" : "SYNCED");
        v["deframer_state"] = deframer_state;
        return v;
    }

    void AquaDBDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Aqua DB Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::BeginGroup();
        {
            // Constellation
            {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                ImVec2 rect_min = ImGui::GetCursorScreenPos();
                ImVec2 rect_max = {rect_min.x + 200 * ui_scale, rect_min.y + 200 * ui_scale};
                draw_list->AddRectFilled(rect_min, rect_max, style::theme.widget_bg);
                draw_list->PushClipRect(rect_min, rect_max);

                for (int i = 0; i < 2048; i++)
                {
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (((int8_t *)buffer)[i * 2 + 0] / 127.0) * 100 * ui_scale) % int(200 * ui_scale),
                                                      ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + (((int8_t *)buffer)[i * 2 + 1] / 127.0) * 100 * ui_scale) % int(200 * ui_scale)),
                                               2 * ui_scale, style::theme.constellation);
                }

                draw_list->PopClipRect();
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

                if (deframer.getState() == deframer.STATE_NOSYNC)
                    ImGui::TextColored(style::theme.red, "NOSYNC");
                else if (deframer.getState() == deframer.STATE_SYNCING)
                    ImGui::TextColored(style::theme.orange, "SYNCING");
                else
                    ImGui::TextColored(style::theme.green, "SYNCED");
            }

            ImGui::Spacing();

            ImGui::Button("Reed-Solomon", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("RS    : ");
                for (int i = 0; i < 4; i++)
                {
                    ImGui::SameLine();

                    if (deframer.getState() == deframer.STATE_NOSYNC)
                    {
                        ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "%i ", i);
                    }
                    else
                    {
                        if (errors[i] == -1)
                            ImGui::TextColored(style::theme.red, "%i ", i);
                        else if (errors[i] > 0)
                            ImGui::TextColored(style::theme.orange, "%i ", i);
                        else
                            ImGui::TextColored(style::theme.green, "%i ", i);
                    }
                }
            }
        }
        ImGui::EndGroup();

        drawProgressBar();

        ImGui::End();
    }

    std::string AquaDBDecoderModule::getID() { return "aqua_db_decoder"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> AquaDBDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<AquaDBDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace aqua