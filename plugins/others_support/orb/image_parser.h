#pragma once

#include <map>
#include <mutex>
#include "common/image/image.h"
#include "common/image/j2k_utils.h"
#include "common/ccsds/ccsds.h"
#include "common/ccsds/ccsds_time.h"
#include "common/utils.h"
#include "imgui/imgui_image.h"
#include <filesystem>

namespace orb
{
    class ImageParser
    {
    public:
        struct ImgStc
        {
            image::Image<uint8_t> img;
            uint32_t textureID = 0;
            uint32_t *textureBuffer;
            int imgsize = 0;
            bool hasToUpdate = false;
            bool is_dling = false;
        }; // TODO, handle changing image sizes!

        std::map<int, ImgStc> decoded_imgs;
        time_t last_timestamp = 0;
        std::mutex savingMutex;

    public:
        std::string directory;

    public:
        ImageParser()
        {
        }

        ~ImageParser()
        {
        }

        void work(ccsds::CCSDSPacket &pkt)
        {
            if (pkt.payload.size() < 18)
            {
                if (pkt.payload.size() == 10) // END?
                {
                    if (pkt.payload[9] == 0xFF)
                    {
                        int channel = pkt.payload[8];
                        saveAll(channel);
                    }
                }

                return;
            }

            image::Image<uint16_t> imgt = image::decompress_j2k_openjp2(&pkt.payload[17], pkt.payload.size() - 17);

            uint64_t timestamp = (uint64_t)pkt.payload[0] << 56 |
                                 (uint64_t)pkt.payload[1] << 48 |
                                 (uint64_t)pkt.payload[2] << 40 |
                                 (uint64_t)pkt.payload[3] << 32 |
                                 (uint64_t)pkt.payload[4] << 24 |
                                 (uint64_t)pkt.payload[5] << 16 |
                                 (uint64_t)pkt.payload[6] << 8 |
                                 (uint64_t)pkt.payload[7]; // TODO use CCSDS
            int channel = pkt.payload[8];
            int width = pkt.payload[9] << 8 | pkt.payload[10];
            int height = pkt.payload[11] << 8 | pkt.payload[12];
            int x_s = pkt.payload[13] << 8 | pkt.payload[14];
            int y_s = pkt.payload[15] << 8 | pkt.payload[16];

            image::Image<uint8_t> img = imgt.to8bits();

            for (size_t i = 0; i < img.size(); i++)
                img[i] = imgt[i];

            if (decoded_imgs.count(channel) == 0)
                decoded_imgs.emplace(channel, ImgStc());

            if ((uint64_t)last_timestamp != timestamp || decoded_imgs[channel].img.size() == 0)
            {
                if (decoded_imgs[channel].img.size() != 0)
                    saveAll();

                // logger->critical("%d %d %d", width, height, channel);
                decoded_imgs[channel].img.init(width, height, 1);
                last_timestamp = timestamp;
            }

            decoded_imgs[channel].img.draw_image(0, img, x_s, y_s);
            decoded_imgs[channel].is_dling = true;

            if (decoded_imgs[channel].textureID != 0)
            {
                auto imageScaled = decoded_imgs[channel].img;
                imageScaled.resize(1000, 1000);
                uchar_to_rgba(imageScaled.data(),
                              decoded_imgs[channel].textureBuffer,
                              imageScaled.height() * imageScaled.width());
                decoded_imgs[channel].hasToUpdate = true;
            }
        }

        void saveAll(int channel = -1)
        {
            std::string dirpath = directory + "/" +
                                  // timestamp_to_string(last_timestamp) +
                                  std::to_string(last_timestamp);

            if (!std::filesystem::exists(dirpath))
                std::filesystem::create_directories(dirpath);

            if (channel == -1)
            {
                for (auto &ch : decoded_imgs)
                {
                    std::string path = dirpath +
                                       "/" +
                                       std::to_string(ch.first);

                    decoded_imgs[ch.first].img.save_img(path);
                    decoded_imgs[ch.first].is_dling = false;
                }
            }
            else if (decoded_imgs.count(channel) > 0)
            {
                std::string path = dirpath +
                                   "/" +
                                   std::to_string(channel);

                decoded_imgs[channel].img.save_img(path);
                decoded_imgs[channel].is_dling = false;
            }
        }
    };
}
