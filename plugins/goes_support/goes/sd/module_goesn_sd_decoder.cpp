#include "module_goesn_sd_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include <volk/volk.h>
#include "common/utils.h"
#include "common/codings/differential/nrzm.h"

#define BUFFER_SIZE 8192
#define SD_FRAME_SIZE 60

namespace goes
{
    namespace sd
    {
        GOESNSDDecoderModule::GOESNSDDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                                      constellation(1.0, 0.15, demod_constellation_size)
        {
            def = std::make_shared<GOESN_SD_Deframer>(480);
            soft_buffer = new int8_t[BUFFER_SIZE];
            soft_bits = new uint8_t[BUFFER_SIZE];
            output_frames = new uint8_t[BUFFER_SIZE];
        }

        std::vector<ModuleDataType> GOESNSDDecoderModule::getInputTypes()
        {
            return {DATA_FILE, DATA_STREAM};
        }

        std::vector<ModuleDataType> GOESNSDDecoderModule::getOutputTypes()
        {
            return {DATA_FILE};
        }

        GOESNSDDecoderModule::~GOESNSDDecoderModule()
        {
            delete[] soft_buffer;
            delete[] soft_bits;
            delete[] output_frames;
        }

        const uint8_t pm_sequence[] = {0xad, 0x43, 0xc4, 0x7e, 0x31, 0x6c, 0x28, 0xae, //
                                       0xde, 0x63, 0xd0, 0x93, 0x2f, 0x10, 0xf0, 0x07, //
                                       0xc2, 0x0e, 0x8c, 0xdf, 0x6b, 0x12, 0xe1, 0x83, //
                                       0x27, 0x56, 0xe3, 0x92, 0xa3, 0xb3, 0xbb, 0xfd, //
                                       0x6e, 0x7b, 0x1a, 0xa7, 0x90, 0xb2, 0x37, 0x5e, //
                                       0xa5, 0x81, 0x36, 0xd2, 0x06, 0xca, 0xcc, 0x7e, //
                                       0x73, 0x5c, 0xb4, 0x05, 0xd3, 0x8a, 0x69, 0x87, //
                                       0x04, 0x5f, 0x29, 0x22};

        void GOESNSDDecoderModule::process()
        {
            if (input_data_type == DATA_FILE)
                filesize = getFilesize(d_input_file);
            else
                filesize = 0;
            if (input_data_type == DATA_FILE)
                data_in = std::ifstream(d_input_file, std::ios::binary);
            data_out = std::ofstream(d_output_file_hint + ".frm", std::ios::binary);
            d_output_files.push_back(d_output_file_hint + ".frm");

            logger->info("Using input symbols " + d_input_file);
            logger->info("Decoding to " + d_output_file_hint + ".frm");

            diff::NRZMDiff diff;

            time_t lastTime = 0;
            while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
            {
                // Read a buffer
                if (input_data_type == DATA_FILE)
                    data_in.read((char *)soft_buffer, BUFFER_SIZE);
                else
                    input_fifo->read((uint8_t *)soft_buffer, BUFFER_SIZE);

                for (int i = 0; i < BUFFER_SIZE; i++)
                    soft_bits[i] = soft_buffer[i] > 0;

                diff.decode_bits(soft_bits, BUFFER_SIZE);

                int nframes = def->work(soft_bits, BUFFER_SIZE, output_frames);

                // Count frames
                // frame_count += nframes;

                // Write to file
                if (nframes > 0)
                {
                    for (int x = 0; x < nframes; x++)
                        for (int i = 0; i < 60; i++)
                            output_frames[x * 60 + i] ^= pm_sequence[i];

                    data_out.write((char *)output_frames, nframes * SD_FRAME_SIZE);
                    if (output_data_type == DATA_FILE)
                        ; //
                    else
                        output_fifo->write(output_frames, nframes * SD_FRAME_SIZE);
                }

                if (input_data_type == DATA_FILE)
                    progress = data_in.tellg();

                if (time(NULL) % 10 == 0 && lastTime != time(NULL))
                {
                    lastTime = time(NULL);
                    std::string deframer_state = def->getState() == def->STATE_NOSYNC ? "NOSYNC" : (def->getState() == def->STATE_SYNCING ? "SYNCING" : "SYNCED");
                    logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%, Deframer : " + deframer_state);
                }
            }

            logger->info("Decoding finished");

            data_out.close();
            if (input_data_type == DATA_FILE)
                data_in.close();
        }

        void GOESNSDDecoderModule::drawUI(bool window)
        {
            ImGui::Begin("GOES-N SD Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

            ImGui::BeginGroup();
            constellation.pushSofttAndGaussian(soft_buffer, 127, BUFFER_SIZE);
            constellation.draw();
            ImGui::EndGroup();

            ImGui::SameLine();

            ImGui::BeginGroup();
            {
                ImGui::Button("Deframer", {200 * ui_scale, 20 * ui_scale});
                {
                    ImGui::Text("State : ");

                    ImGui::SameLine();

                    if (def->getState() == def->STATE_NOSYNC)
                        ImGui::TextColored(IMCOLOR_NOSYNC, "NOSYNC");
                    else if (def->getState() == def->STATE_SYNCING)
                        ImGui::TextColored(IMCOLOR_SYNCING, "SYNCING");
                    else
                        ImGui::TextColored(IMCOLOR_SYNCED, "SYNCED");
                }
            }
            ImGui::EndGroup();

            if (!streamingInput)
                ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

            ImGui::End();
        }

        std::string GOESNSDDecoderModule::getID()
        {
            return "goesn_sd_decoder";
        }

        std::vector<std::string> GOESNSDDecoderModule::getParameters()
        {
            return {};
        }

        std::shared_ptr<ProcessingModule> GOESNSDDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GOESNSDDecoderModule>(input_file, output_file_hint, parameters);
        }
    }
}