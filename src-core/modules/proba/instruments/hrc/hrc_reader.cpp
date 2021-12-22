#include "hrc_reader.h"
#include <fstream>
#include <iostream>
#include <map>

#define WRITE_IMAGE_LOCAL(image, path)         \
    image.save_png(std::string(path).c_str()); \
    all_images.push_back(path);

namespace proba
{
    namespace hrc
    {
        HRCReader::HRCReader(std::string &outputfolder)
        {
            tempChannelBuffer = new unsigned short[74800 * 12096];
            frame_count = 0;
            count = 0;
            output_folder = outputfolder;
        }

        HRCReader::~HRCReader()
        {
            delete[] tempChannelBuffer;
        }

        uint8_t reverseBits(uint8_t byte)
        {
            byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
            byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
            byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
            return byte;
        }

        uint16_t reverse16Bits(uint16_t v)
        {
            uint16_t r = 0;
            for (int i = 0; i < 16; ++i, v >>= 1)
                r = (r << 1) | (v & 0x01);
            return r;
        }

        void HRCReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 21458)
                return;

            int count_marker = packet.payload[18 - 6];

            int pos = 21;

            // Convert into 10-bits values
            for (int i = 0; i < 17152; i += 4)
            {
                if (count_marker <= 65)
                {
                    tempChannelBuffer[count_marker * 17152 + i + 0] = reverse16Bits((reverseBits(packet.payload[pos + 0]) << 2) | (reverseBits(packet.payload[pos + 1]) >> 6));
                    tempChannelBuffer[count_marker * 17152 + i + 1] = reverse16Bits(((reverseBits(packet.payload[pos + 1]) % 64) << 4) | (reverseBits(packet.payload[pos + 2]) >> 4));
                    tempChannelBuffer[count_marker * 17152 + i + 2] = reverse16Bits(((reverseBits(packet.payload[pos + 2]) % 16) << 6) | (reverseBits(packet.payload[pos + 3]) >> 2));
                    tempChannelBuffer[count_marker * 17152 + i + 3] = reverse16Bits(((reverseBits(packet.payload[pos + 3]) % 4) << 8) | reverseBits(packet.payload[pos + 4]));
                    pos += 5;
                }
            }

            frame_count++;

            if (count_marker == 65)
            {
                save();
                frame_count = 0;
            }
        }

        void HRCReader::save()
        {
            if (frame_count != 0)
            {
                std::cout << "Finished HRC image! Saving as HRC-" + std::to_string(count) + ".png" << std::endl;
                image::Image<uint16_t> img = image::Image<uint16_t>(tempChannelBuffer, 1072, 1072, 1);
                img.normalize();
                WRITE_IMAGE_LOCAL(img, output_folder + "/HRC-" + std::to_string(count) + ".png");
                img.equalize();
                WRITE_IMAGE_LOCAL(img, output_folder + "/HRC-" + std::to_string(count) + "-EQU.png");

                std::fill(&tempChannelBuffer[0], &tempChannelBuffer[748 * 12096], 0);
                count++;
            }
        }
    } // namespace hrc
} // namespace proba