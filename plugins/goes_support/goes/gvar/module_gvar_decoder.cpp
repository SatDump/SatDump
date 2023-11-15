#include "module_gvar_decoder.h"
#include "logger.h"
#include "common/codings/differential/nrzs.h"
#include "imgui/imgui.h"
#include "gvar_deframer.h"
#include "gvar_derand.h"

#define BUFFER_SIZE 8192

// Return filesize
uint64_t getFilesize(std::string filepath);

namespace goes
{
    namespace gvar
    {
        GVARDecoderModule::GVARDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
        {
            buffer = new int8_t[BUFFER_SIZE];
        }

        std::vector<ModuleDataType> GVARDecoderModule::getInputTypes()
        {
            return {DATA_FILE, DATA_STREAM};
        }

        std::vector<ModuleDataType> GVARDecoderModule::getOutputTypes()
        {
            return {DATA_FILE, DATA_STREAM};
        }

        GVARDecoderModule::~GVARDecoderModule()
        {
            delete[] buffer;
        }

        void GVARDecoderModule::process()
        {
            if (input_data_type == DATA_FILE)
                filesize = getFilesize(d_input_file);
            else
                filesize = 0;
            if (input_data_type == DATA_FILE)
                data_in = std::ifstream(d_input_file, std::ios::binary);
            if (output_data_type == DATA_FILE)
            {
                data_out = std::ofstream(d_output_file_hint + ".gvar", std::ios::binary);
                d_output_files.push_back(d_output_file_hint + ".gvar");
            }

            logger->info("Using input symbols " + d_input_file);
            logger->info("Decoding to " + d_output_file_hint + ".gvar");

            time_t lastTime = 0;

            diff::NRZSDiff diff;

            // The deframer
            GVARDeframer<uint64_t, 64, 262288, 0b0001101111100111110100000001111110111111100000001111111111111110> deframer;

            // Derand
            PNDerandomizer derand;

            // Final buffer after decoding
            uint8_t finalBuffer[BUFFER_SIZE];

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
                        derand.derandData(frame.data(), 32786);

                        if (output_data_type == DATA_FILE)
                            data_out.write((char *)frame.data(), 32786);
                        else
                            output_fifo->write(frame.data(), 32786);
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

            if (output_data_type == DATA_FILE)
                data_out.close();
            if (input_data_type == DATA_FILE)
                data_in.close();
        }

        void GVARDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("GVAR Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

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
                        draw_list->AddCircleFilled(ImVec2(ImGui::GetCursorScreenPos().x + (int)(100 * ui_scale + (buffer[i] / 127.0) * 130 * ui_scale) % int(200 * ui_scale),
                                                          ImGui::GetCursorScreenPos().y + (int)(100 * ui_scale + rng.gasdev() * 14 * ui_scale) % int(200 * ui_scale)),
                                                   2 * ui_scale,
                                                   ImColor::HSV(113.0 / 360.0, 1, 1, 1.0));
                    }

                    ImGui::Dummy(ImVec2(200 * ui_scale + 3, 200 * ui_scale + 3));
                }
            }
            ImGui::EndGroup();

            if (!streamingInput)
                ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string GVARDecoderModule::getID()
        {
            return "goes_gvar_decoder";
        }

        std::vector<std::string> GVARDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> GVARDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GVARDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace elektro
}