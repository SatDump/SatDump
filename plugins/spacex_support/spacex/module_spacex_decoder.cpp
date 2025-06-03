#include "module_spacex_decoder.h"
#include "common/codings/randomization.h"
#include "common/codings/reedsolomon/reedsolomon.h"
#include "common/codings/rotation.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <cstdint>

#define BUFFER_SIZE 8192 * 10

// Returns the asked bit!
template <typename T>
inline bool getBit(T data, int bit)
{
    return (data >> bit) & 1;
}

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace spacex
{
    SpaceXDecoderModule::SpaceXDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), qpsk(parameters["qpsk"].get<bool>())
    {
        buffer = new int8_t[BUFFER_SIZE];
        fsfsm_file_ext = ".cadu";
    }

    SpaceXDecoderModule::~SpaceXDecoderModule() { delete[] buffer; }

    void SpaceXDecoderModule::process()
    {
        reedsolomon::ReedSolomon rs(reedsolomon::RS239);

        // Final buffer after decoding
        uint8_t finalBuffer[BUFFER_SIZE];

        // Bits => Bytes stuff
        uint8_t byteShifter = 0;
        int inByteShifter = 0;
        int byteShifted = 0;

        // PacketFixer
        int unsynced_run = 0;
        int ph = PHASE_0;
        bool swap = false;

        while (should_run())
        {
            // Read buffer
            read_data((uint8_t *)buffer, BUFFER_SIZE);

            if (qpsk)
                rotate_soft((int8_t *)buffer, BUFFER_SIZE, (phase_t)ph, swap);

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

            if (qpsk)
            {
                if (deframer.getState() == 0)
                {
                    unsynced_run++;
                    if (unsynced_run == 10)
                    {
                        ph++;
                        if (ph == 4)
                        {
                            ph = 0;
                            swap = !swap;
                        }
                        unsynced_run = 0;
                    }
                }
                else
                {
                    unsynced_run = 0;
                }
            }

            // Deframe that! (Integrated derand)
            std::vector<std::array<uint8_t, ccsds::ccsds_tm::CADU_SIZE>> frameBuffer = deframer.work(finalBuffer, (BUFFER_SIZE / 8));

            // If we found frames, write them out
            if (frameBuffer.size() > 0)
            {
                for (std::array<uint8_t, ccsds::ccsds_tm::CADU_SIZE> cadu : frameBuffer)
                {
                    // RS Correction
                    rs.decode_interlaved(&cadu[4], true, 5, errors);

                    derand_ccsds(&cadu[4], ccsds::ccsds_tm::CADU_SIZE - 4);

                    if (errors[0] > -1 && errors[1] > -1 && errors[2] > -1 && errors[3] > -1 && errors[4] > -1)
                        write_data((uint8_t *)&cadu, ccsds::ccsds_tm::CADU_SIZE);
                }
            }
        }

        cleanup();
    }

    nlohmann::json SpaceXDecoderModule::getModuleStats()
    {
        auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
        std::string deframer_state = deframer.getState() == 0 ? "NOSYNC" : (deframer.getState() == 2 || deframer.getState() == 6 ? "SYNCING" : "SYNCED");
        v["deframer_state"] = deframer_state;
        return v;
    }

    void SpaceXDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("SpaceX TLM Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

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
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (buffer[i] / 127.0) * 100 * ui_scale) % int(200 * ui_scale),
                                                      ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + rng.gasdev() * 6 * ui_scale) % int(200 * ui_scale)),
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

                if (deframer.getState() == 0)
                    ImGui::TextColored(style::theme.red, "NOSYNC");
                else if (deframer.getState() == 2 || deframer.getState() == 6)
                    ImGui::TextColored(style::theme.orange, "SYNCING");
                else
                    ImGui::TextColored(style::theme.green, "SYNCED");
            }

            ImGui::Spacing();

            ImGui::Button("Reed-Solomon", {200 * ui_scale, 20 * ui_scale});
            {
                ImGui::Text("RS    : ");
                for (int i = 0; i < 5; i++)
                {
                    ImGui::SameLine();

                    if (errors[i] == -1)
                        ImGui::TextColored(style::theme.red, "%i ", i);
                    else if (errors[i] > 0)
                        ImGui::TextColored(style::theme.orange, "%i ", i);
                    else
                        ImGui::TextColored(style::theme.green, "%i ", i);
                }
            }
        }
        ImGui::EndGroup();

        drawProgressBar();

        ImGui::End();
    }

    std::string SpaceXDecoderModule::getID() { return "spacex_tlm_decoder"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> SpaceXDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<SpaceXDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace spacex