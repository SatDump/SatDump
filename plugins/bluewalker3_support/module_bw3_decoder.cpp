#include "module_bw3_decoder.h"
#include "common/utils.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace bluewalker3
{
    BW3DecoderModule::BW3DecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : satdump::pipeline::base::FileStreamToFileStreamModule(input_file, output_file_hint, parameters), d_cadu_size(parameters["cadu_size"].get<int>()),
          d_payload_size(parameters["payload_size"].get<int>())
    {
        fsfsm_enable_output = false;
    }

    void BW3DecoderModule::process()
    {
        std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

        uint8_t *buffer = new uint8_t[d_cadu_size];

        std::vector<uint8_t> wip_pkt;
        std::vector<uint8_t> wip_pictures_stream;

        while (should_run())
        {
            // Read buffer
            read_data((uint8_t *)buffer, d_cadu_size);

            uint8_t frame_type = buffer[5];

            // printf("MARKER %d\n", frame_type);

            if (frame_type == 217)
            {
                // uint16_t frm_id = buffer[18] << 8 | buffer[17];
                uint16_t ptr = buffer[20] << 8 | buffer[19];

                // printf("PTR %d %d\n", ptr, ptr2);

                if (ptr == d_payload_size)
                {
                    wip_pkt.insert(wip_pkt.end(), &buffer[21], &buffer[21 + d_payload_size]);
                }
                else if (ptr < d_payload_size)
                {
                    wip_pkt.insert(wip_pkt.end(), &buffer[21], &buffer[21 + ptr]);
                    wip_pkt.erase(wip_pkt.begin(), wip_pkt.begin() + 8);

                    wip_pictures_stream.insert(wip_pictures_stream.end(), wip_pkt.begin(), wip_pkt.end());

                    wip_pkt.clear();
                    wip_pkt.insert(wip_pkt.end(), &buffer[21 + ptr], &buffer[21 + d_payload_size]);
                }
            }
        }

        delete[] buffer;
        cleanup();

        uint8_t sliding_header[4] = {0, 0, 0, 0};
        std::vector<uint8_t> wip_until_hdr;
        int img_cnt = 1;

        for (uint8_t b : wip_pictures_stream)
        {
            sliding_header[0] = sliding_header[1];
            sliding_header[1] = sliding_header[2];
            sliding_header[2] = sliding_header[3];
            sliding_header[3] = b;

            wip_until_hdr.push_back(b);

            if (sliding_header[0] == 0xFF && sliding_header[1] == 0xD8 && sliding_header[2] == 0xFF && sliding_header[3] == 0xE0)
            {
                logger->info("Writing image %d", img_cnt);
                std::ofstream raw_img(directory + "BlueWaker3_Cam_" + std::to_string(img_cnt++) + ".jpg", std::ios::binary);
                raw_img.put(0xFF);
                raw_img.put(0xD8);
                raw_img.put(0xFF);
                raw_img.write((char *)wip_until_hdr.data(), wip_until_hdr.size());
                raw_img.close();
                wip_until_hdr.clear();
            }
        }

        if (wip_until_hdr.size() > 0)
        {
            logger->info("Writing image %d", img_cnt);
            std::ofstream raw_img(directory + "BlueWaker3_Cam_" + std::to_string(img_cnt++) + ".jpg", std::ios::binary);
            raw_img.put(0xFF);
            raw_img.put(0xD8);
            raw_img.put(0xFF);
            raw_img.write((char *)wip_until_hdr.data(), wip_until_hdr.size());
            raw_img.close();
        }
    }

    void BW3DecoderModule::drawUI(bool window)
    {
        ImGui::Begin("BlueWalker-3 Decoder", NULL, window ? 0 : NOWINDOW_FLAGS);

        drawProgressBar();

        ImGui::End();
    }

    std::string BW3DecoderModule::getID() { return "bw3_decoder"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> BW3DecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<BW3DecoderModule>(input_file, output_file_hint, parameters);
    }
} // namespace bluewalker3