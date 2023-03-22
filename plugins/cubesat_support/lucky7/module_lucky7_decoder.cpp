#include "module_lucky7_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/image/image.h"

namespace lucky7
{
    Lucky7DecoderModule::Lucky7DecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        frame_buffer = new uint8_t[35];
    }

    Lucky7DecoderModule::~Lucky7DecoderModule()
    {
        delete[] frame_buffer;
    }

    void Lucky7DecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        std::ifstream data_in(d_input_file, std::ios::binary);

        std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

        logger->info("Using input frames " + d_input_file);
        logger->info("Decoding to " + directory);

        std::map<int, ImagePayload> wip_image_payloads;

        time_t lastTime = 0;
        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read buffer
            data_in.read((char *)frame_buffer, 35);

            if (frame_buffer[1] == 0x00) // TLM
            {
                std::string callsign(&frame_buffer[6], &frame_buffer[12]);
                logger->info("Telemetry " + callsign);
            }
            else if ((frame_buffer[1] & 0xC0) == 0xC0) // Imagery
            {
                uint16_t current_chunk = (frame_buffer[1] & 0xF) << 8 | frame_buffer[2];
                uint16_t total_chunks = frame_buffer[5] << 8 | frame_buffer[6];
                // logger->info("{:d}/{:d}", current_chunk, total_chunks);

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

            progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((float)progress / (float)filesize) * 1000.0f) / 10.0f) + "%");
            }
        }

        logger->info("Decoding finished");

        data_in.close();

        for (auto imgp : wip_image_payloads)
        {
            logger->info("Image Payload {:d}. Got {:d}/{:d}", imgp.first, imgp.second.get_present(), imgp.second.total_chunks);

            image::Image<uint8_t> img;
            img.load_jpeg(imgp.second.payload.data(), imgp.second.payload.size());

            std::string name = "Img_" + std::to_string(imgp.first) + ".png";

            logger->info("Saving to " + name);

            img.save_png(directory + "/" + name);
        }
    }

    void Lucky7DecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Lucky-7 Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        if (!streamingInput)
            ImGui::ProgressBar((float)progress / (float)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string Lucky7DecoderModule::getID()
    {
        return "lucky7_decoder";
    }

    std::vector<std::string> Lucky7DecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> Lucky7DecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<Lucky7DecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace noaa
