#include "module_lucky7_decoder.h"
#include "common/utils.h"
#include "image/io.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <cstdint>

namespace lucky7
{
    Lucky7DecoderModule::Lucky7DecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters)
    {
        frame_buffer = new uint8_t[35];
        fsfsm_enable_output = false;
    }

    Lucky7DecoderModule::~Lucky7DecoderModule() { delete[] frame_buffer; }

    void Lucky7DecoderModule::process()
    {
        std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

        std::map<int, ImagePayload> wip_image_payloads;

        while (should_run())
        {
            // Read buffer
            read_data((uint8_t *)frame_buffer, 35);

            if (frame_buffer[1] == 0x00) // TLM
            {
                std::string callsign(&frame_buffer[6], &frame_buffer[12]);
                logger->info("Telemetry " + callsign);
            }
            else if ((frame_buffer[1] & 0xC0) == 0xC0) // Imagery
            {
                uint16_t current_chunk = (frame_buffer[1] & 0xF) << 8 | frame_buffer[2];
                uint16_t total_chunks = frame_buffer[5] << 8 | frame_buffer[6];
                // logger->info("%d/%d", current_chunk, total_chunks);

                if (wip_image_payloads.count(total_chunks) == 0)
                {
                    ImagePayload newp;
                    newp.total_chunks = total_chunks + 1; // Need to account for 0
                    newp.has_chunks = std::vector<bool>(newp.total_chunks, false);
                    newp.payload.resize(newp.total_chunks * 28);
                    wip_image_payloads.insert({total_chunks, newp});
                }

                ImagePayload &payload = wip_image_payloads[total_chunks];

                memcpy(&payload.payload[current_chunk * 28], &frame_buffer[7], 28);
                payload.has_chunks[current_chunk] = true;
            }
            else if ((frame_buffer[1] & 0x20) == 0x20) // Dosimetry?
            {
            }
        }

        logger->info("Decoding finished");

        cleanup();

        for (auto imgp : wip_image_payloads)
        {
            logger->info("Image Payload %d. Got %d/%d", imgp.first, imgp.second.get_present(), imgp.second.total_chunks);

            image::Image img;
            image::load_jpeg(img, imgp.second.payload.data(), imgp.second.payload.size());

            std::string name = "Img_" + std::to_string(imgp.first);
            image::save_img(img, directory + "/" + name);
        }
    }

    void Lucky7DecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Lucky-7 Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        drawProgressBar();

        ImGui::End();
    }

    std::string Lucky7DecoderModule::getID() { return "lucky7_decoder"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> Lucky7DecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<Lucky7DecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace lucky7
