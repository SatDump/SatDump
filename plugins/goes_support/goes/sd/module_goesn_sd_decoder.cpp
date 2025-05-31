#include "module_goesn_sd_decoder.h"
#include "common/codings/differential/nrzm.h"
#include "common/utils.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <volk/volk.h>

#define BUFFER_SIZE 8192
#define SD_FRAME_SIZE 60

namespace goes
{
    namespace sd
    {
        GOESNSDDecoderModule::GOESNSDDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
            : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), constellation(1.0, 0.15, demod_constellation_size)
        {
            def = std::make_shared<GOESN_SD_Deframer>(480);
            soft_buffer = new int8_t[BUFFER_SIZE];
            soft_bits = new uint8_t[BUFFER_SIZE];
            output_frames = new uint8_t[BUFFER_SIZE];
            fsfsm_file_ext = ".frm";
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
            diff::NRZMDiff diff;

            while (should_run())
            {
                // Read a buffer
                read_data((uint8_t *)soft_buffer, BUFFER_SIZE);

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

                    write_data(output_frames, nframes * SD_FRAME_SIZE);
                }
            }

            logger->info("Decoding finished");

            cleanup();
        }

        nlohmann::json GOESNSDDecoderModule::getModuleStats()
        {
            auto v = satdump::pipeline::base::FileStreamToFileStreamModule::getModuleStats();
            std::string deframer_state = def->getState() == def->STATE_NOSYNC ? "NOSYNC" : (def->getState() == def->STATE_SYNCING ? "SYNCING" : "SYNCED");
            v["deframer_state"] = deframer_state;
            return v;
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
                        ImGui::TextColored(style::theme.red, "NOSYNC");
                    else if (def->getState() == def->STATE_SYNCING)
                        ImGui::TextColored(style::theme.orange, "SYNCING");
                    else
                        ImGui::TextColored(style::theme.green, "SYNCED");
                }
            }
            ImGui::EndGroup();

            drawProgressBar();

            ImGui::End();
        }

        std::string GOESNSDDecoderModule::getID() { return "goesn_sd_decoder"; }

        std::shared_ptr<satdump::pipeline::ProcessingModule> GOESNSDDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        {
            return std::make_shared<GOESNSDDecoderModule>(input_file, output_file_hint, parameters);
        }
    } // namespace sd
} // namespace goes