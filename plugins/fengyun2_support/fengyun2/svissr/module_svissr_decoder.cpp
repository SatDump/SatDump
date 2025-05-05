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
    SVISSRDecoderModule::SVISSRDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        buffer = new int8_t[BUFFER_SIZE];

        pn_sync = parameters["pn_sync"];
    }

    std::vector<ModuleDataType> SVISSRDecoderModule::getInputTypes() { return {DATA_FILE, DATA_STREAM}; }

    std::vector<ModuleDataType> SVISSRDecoderModule::getOutputTypes() { return {DATA_FILE, DATA_STREAM}; }

    SVISSRDecoderModule::~SVISSRDecoderModule() { delete[] buffer; }

    void SVISSRDecoderModule::process()
    {
        if (input_data_type == DATA_FILE)
            filesize = getFilesize(d_input_file);
        else
            filesize = 0;
        if (input_data_type == DATA_FILE)
            data_in = std::ifstream(d_input_file, std::ios::binary);
        if (output_data_type == DATA_FILE)
        {
            data_out = std::ofstream(d_output_file_hint + ".svissr", std::ios::binary);
            d_output_files.push_back(d_output_file_hint + ".svissr");
        }

        logger->info("Using input symbols " + d_input_file);
        logger->info("Decoding to " + d_output_file_hint + ".svissr");

        time_t lastTime = 0;

        diff::NRZMDiff diff;

        // The deframer
        SVISSRDeframer deframer;

        // Derand
        PNDerandomizer derand;

        // PN Sync
        PNSync sync(BUFFER_SIZE);
        int8_t *buffer2 = buffer;

        // Final buffer after decoding
        uint8_t *finalBuffer = new uint8_t[sync.TARGET_WROTE_BITS / 8];

        // Bits => Bytes stuff
        uint8_t byteShifter = 0;
        int inByteShifter = 0;
        int byteShifted = 0;

        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read a buffer
            if (input_data_type == DATA_FILE)
                data_in.read((char *)buffer, BUFFER_SIZE);
            else
                input_fifo->read((uint8_t *)buffer, BUFFER_SIZE);

            int ret = pn_sync ? sync.process((const uint8_t *)buffer, BUFFER_SIZE, (uint8_t **)&buffer2) : BUFFER_SIZE;

            // Group symbols into bytes now, I channel
            inByteShifter = 0;
            byteShifted = 0;

            for (int i = 0; i < ret; i++)
            {
                byteShifter = byteShifter << 1 | (buffer2[i] > 0);
                inByteShifter++;

                if (inByteShifter == 8)
                {
                    finalBuffer[byteShifted++] = byteShifter;
                    inByteShifter = 0;
                }
            }

            // Differential decoding for both of them
            diff.decode(finalBuffer, ret / 8);

            // Deframe
            std::vector<std::vector<uint8_t>> frameBuffer = deframer.work(finalBuffer, ret / 8);

            // If we found frames, write them out
            if (frameBuffer.size() > 0)
            {
                for (std::vector<uint8_t> &frame : frameBuffer)
                {
                    derand.derandData(frame.data(), 44356);

                    if (output_data_type == DATA_FILE)
                        data_out.write((char *)frame.data(), 44356);
                    else
                        output_fifo->write(frame.data(), 44356);
                }
            }

            if (input_data_type == DATA_FILE)
                progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
            }
        }

        delete[] finalBuffer;

        if (output_data_type == DATA_FILE)
            data_out.close();
        if (input_data_type == DATA_FILE)
            data_in.close();
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

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

        ImGui::End();
    }

    std::string SVISSRDecoderModule::getID() { return "fengyun_svissr_decoder"; }

    std::vector<std::string> SVISSRDecoderModule::getParameters() { return {}; }

    std::shared_ptr<ProcessingModule> SVISSRDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<SVISSRDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace fengyun_svissr