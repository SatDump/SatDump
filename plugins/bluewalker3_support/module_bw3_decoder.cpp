#include "module_bw3_decoder.h"
#include <fstream>
#include "logger.h"
#include <filesystem>
#include "imgui/imgui.h"
#include "common/utils.h"

namespace bluewalker3
{
    BW3DecoderModule::BW3DecoderModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters),
                                                                                                                          d_cadu_size(parameters["cadu_size"].get<int>()),
                                                                                                                          d_payload_size(parameters["payload_size"].get<int>())
    {
    }

    void BW3DecoderModule::process()
    {
        filesize = getFilesize(d_input_file);
        std::ifstream data_in(d_input_file, std::ios::binary);

        std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

        logger->info("Using input frames " + d_input_file);
        logger->info("Decoding to " + directory);

        time_t lastTime = 0;

        uint8_t *buffer = new uint8_t[d_cadu_size];

        std::vector<uint8_t> wip_pkt;
        std::vector<uint8_t> wip_pictures_stream;

        while (!data_in.eof())
        {
            // Read buffer
            data_in.read((char *)buffer, d_cadu_size);

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

            progress = data_in.tellg();

            if (time(NULL) % 10 == 0 && lastTime != time(NULL))
            {
                lastTime = time(NULL);
                logger->info("Progress " + std::to_string(round(((double)progress / (double)filesize) * 1000.0) / 10.0) + "%%");
            }
        }

        delete[] buffer;
        data_in.close();

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

        ImGui::ProgressBar((double)progress / (double)filesize, ImVec2(ImGui::GetWindowWidth() - 10, 20 * ui_scale));

        ImGui::End();
    }

    std::string BW3DecoderModule::getID()
    {
        return "bw3_decoder";
    }

    std::vector<std::string> BW3DecoderModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> BW3DecoderModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<BW3DecoderModule>(input_file, output_file_hint, parameters);
    }
}