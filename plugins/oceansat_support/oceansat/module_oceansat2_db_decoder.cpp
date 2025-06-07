#include "module_oceansat2_db_decoder.h"
#include "common/codings/differential/generic.h"
#include "common/utils.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "oceansat2_deframer.h"
#include "oceansat2_derand.h"
#include <cstdint>

#define BUFFER_SIZE 8192

namespace oceansat
{
    // Modified to only repack the OCM channel
    class RepackOneChannel
    {
    private:
        uint8_t byteToWrite = 0;
        int inByteToWrite = 0;

    public:
        int work(uint8_t *in, int length, uint8_t *out)
        {
            int oo = 0;

            for (int ii = 0; ii < length; ii++)
            {
                byteToWrite = byteToWrite << 1 | ((in[ii] >> 1) & 1);
                inByteToWrite++;

                if (inByteToWrite == 8)
                {
                    out[oo++] = byteToWrite;
                    inByteToWrite = 0;
                }
            }

            return oo;
        }
    };

    Oceansat2DBDecoderModule::Oceansat2DBDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        buffer = new int8_t[BUFFER_SIZE];
        frame_count = 0;
        fsfsm_file_ext = ".ocm";
    }

    Oceansat2DBDecoderModule::~Oceansat2DBDecoderModule() { delete[] buffer; }

    void Oceansat2DBDecoderModule::process()
    {
        // I/Q Buffers
        uint8_t decodedBuffer1[BUFFER_SIZE], decodedBuffer2[BUFFER_SIZE];
        uint8_t diffedBuffer1[BUFFER_SIZE], diffedBuffer2[BUFFER_SIZE];

        // Final buffer after decoding
        uint8_t finalBuffer1[BUFFER_SIZE], finalBuffer2[BUFFER_SIZE];

        // Bits => Bytes stuff
        RepackOneChannel repack1, repack2;

        diff::GenericDiff diffdecoder1(4), diffdecoder2(4);

        Oceansat2Deframer deframer1, deframer2;
        Oceansat2Derand derand;

        // In this decoder both swapped and normal I/Q are always decoded. This does it crappier
        // than correlation, but works well enough and faster. And anyway, we aren't dealing with
        // that much data.
        // We can also safely assume frames will NEVER be found on both states in some weird way...
        // And that, even if the second channel did contain some data, which it doesn't since only
        // the OCM instrument is still alive on OceanSat-2
        // So I guess this is fine enough?

        while (should_run())
        {
            // Read buffer
            read_data((uint8_t *)buffer, BUFFER_SIZE);

            // Demodulate DQPSK... This is the crappy way but it works
            for (int i = 0; i < BUFFER_SIZE / 2; i++)
            {
                uint8_t demod = dqpsk_demod(&buffer[i * 2]);
                decodedBuffer1[i] = demod;
                decodedBuffer2[i] = (demod >> 1) | (demod & 1) << 1;
            }

            int diff_count1 = diffdecoder1.work(decodedBuffer1, BUFFER_SIZE / 2, diffedBuffer1); // Diff normal IQ
            int diff_count2 = diffdecoder2.work(decodedBuffer2, BUFFER_SIZE / 2, diffedBuffer2); // Diff inversed IQ

            int byteCount1 = repack1.work(diffedBuffer1, diff_count1, finalBuffer1); // Repack normal IQ
            int byteCount2 = repack2.work(diffedBuffer2, diff_count2, finalBuffer2); // Repack inversed IQ

            std::vector<std::vector<uint8_t>> frames1 = deframer1.work(finalBuffer1, byteCount1); // Deframe normal IQ
            std::vector<std::vector<uint8_t>> frames2 = deframer2.work(finalBuffer2, byteCount2); // Deframe inversed IQ

            frame_count += frames1.size() + frames2.size();

            // Process normal IQ
            for (std::vector<uint8_t> frame : frames1)
            {
                derand.work(frame.data());
                write_data((uint8_t *)frame.data(), 92220);
            }

            // Process inversed IQ
            for (std::vector<uint8_t> frame : frames2)
            {
                derand.work(frame.data());
                write_data((uint8_t *)frame.data(), 92220);
            }

            // This above, just processing without checking what frame came first is only fine because we are using a smaller
            // buffer size than frame size. First will get out first.
        }

        cleanup();
    }

    nlohmann::json Oceansat2DBDecoderModule::getModuleStats()
    {
        auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
        v["frame_count"] = frame_count;
        return v;
    }

    void Oceansat2DBDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Oceansat-2 DB Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

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
                ImGui::Text("Frames : ");
                ImGui::SameLine();
                ImGui::TextColored(style::theme.green, UITO_C_STR(frame_count));
            }
        }
        ImGui::EndGroup();

        drawProgressBar();

        ImGui::End();
    }

    std::string Oceansat2DBDecoderModule::getID() { return "oceansat2_db_decoder"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> Oceansat2DBDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<Oceansat2DBDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace oceansat