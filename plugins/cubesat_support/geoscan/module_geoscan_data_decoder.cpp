#include "module_geoscan_data_decoder.h"
#include "logger.h"
#include "imgui/imgui.h"
#include "common/utils.h"
#include "common/image/image.h"

namespace geoscan
{
    GEOSCANDataDecoderModule::GEOSCANDataDecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        frame_buffer = new uint8_t[70];
    }

    GEOSCANDataDecoderModule::~GEOSCANDataDecoderModule()
    {
        delete[] frame_buffer;
    }

    void GEOSCANDataDecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        std::ifstream data_in(d_input_file, std::ios::binary);

        std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

        logger->info("Using input frames " + d_input_file);
        logger->info("Decoding to " + directory);

        std::vector<bool> has_chunks(878, false);
        std::vector<uint8_t> img_vector(878 * 56, 0);

        time_t lastTime = 0;
        while (input_data_type == DATA_FILE ? !data_in.eof() : input_active.load())
        {
            // Read buffer
            data_in.read((char *)frame_buffer, 70);

            if (frame_buffer[4] == 0x84) // TLM
            {
                logger->error("Telemetry Packet. TODO : IMPLEMENT!");
            }
            else if (frame_buffer[4] == 0x02 || frame_buffer[4] == 0x01) // Imagery
            {
                // uint16_t hdr = frame_buffer[4 + 1] << 8 | frame_buffer[4 + 0];
                // uint8_t data_size = frame_buffer[4 + 2];
                // uint16_t msg_type = frame_buffer[4 + 4] << 8 | frame_buffer[4 + 3];
                uint16_t pkt_offset = frame_buffer[4 + 6] << 8 | frame_buffer[4 + 5];

                if (pkt_offset <= 49112)
                {
                    memcpy(&img_vector[pkt_offset], &frame_buffer[4 + 8], 56);
                    has_chunks[pkt_offset / 56] = true;
                }

                // logger->trace("%d", pkt_offset / 56);
            }

            progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
            }
        }

        logger->info("Decoding finished");

        data_in.close();

        int cpresent = 0;
        for (bool v : has_chunks)
            cpresent += v;

        logger->info("Image Payload : Got %d/%d", cpresent, 878);

        image::Image<uint8_t> img;
        img.load_jpeg(img_vector.data(), img_vector.size());

        std::string name = "Image";
        img.save_img(directory + "/" + name);
    }

    void GEOSCANDataDecoderModule::drawUI(bool window)
    {
        ImGui::Begin("Geoscan Data Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        if (!streamingInput)
            ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetContentRegionAvail().x, 20 * ui_scale));

        ImGui::End();
    }

    std::string GEOSCANDataDecoderModule::getID()
    {
        return "geoscan_data_decoder";
    }

    std::vector<std::string> GEOSCANDataDecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> GEOSCANDataDecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<GEOSCANDataDecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace noaa
