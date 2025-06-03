#include "module_svissr_decoder.h"
#include "common/codings/differential/nrzm.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "svissr_deframer.h"
#include "svissr_derand.h"
#include <cstdint>
#include <cstdio>

#define BUFFER_SIZE 8192

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace fengyun_svissr
{
    SVISSRDecoderModule::SVISSRDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        buffer = new int8_t[BUFFER_SIZE];
        fsfsm_file_ext = ".svissr";
    }

    SVISSRDecoderModule::~SVISSRDecoderModule() { delete[] buffer; }

    void SVISSRDecoderModule::process()
    {
        diff::NRZMDiff diff;

        // The deframer
        SVISSRDeframer deframer;

        // Derand
        PNDerandomizer derand;

        // Final buffer after decoding
        uint8_t *finalBuffer = new uint8_t[BUFFER_SIZE];

        // Bits => Bytes stuff
        uint8_t byteShifter = 0;
        int inByteShifter = 0;
        int byteShifted = 0;

        while (should_run())
        {
            // Read a buffer
            read_data((uint8_t *)buffer, BUFFER_SIZE);

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

            // Differential decoding for both of them
            diff.decode(finalBuffer, BUFFER_SIZE / 8);

            // Deframe
            std::vector<std::vector<uint8_t>> frameBuffer = deframer.work(finalBuffer, BUFFER_SIZE / 8);

            // If we found frames, write them out
            if (frameBuffer.size() > 0)
            {
                for (std::vector<uint8_t> &frame : frameBuffer)
                {
                    derand.derandData(frame.data(), 44356);

                    write_data((uint8_t *)frame.data(), 44356);
                }
            }
        }

        delete[] finalBuffer;

        cleanup();
    }

    void SVISSRDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("S-VISSR Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

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
                    draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (buffer[i] / 127.0) * 130 * ui_scale) % int(200 * ui_scale),
                                                      ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + rng.gasdev() * 14 * ui_scale) % int(200 * ui_scale)),
                                               2 * ui_scale, style::theme.constellation);
                }

                draw_list->PopClipRect();
                ImGui::Dummy(ImVec2(200 * ui_scale + 3, 200 * ui_scale + 3));
            }
        }
        ImGui::EndGroup();

        drawProgressBar();

        ImGui::End();
    }

    std::string SVISSRDecoderModule::getID() { return "fengyun_svissr_decoder"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> SVISSRDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<SVISSRDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace fengyun_svissr