#include "hrc_reader.h"
#include "../crc.h"
#include "image/io.h"
#include "image/processing.h"
#include "logger.h"
#include "utils/binary.h"

namespace proba
{
    namespace hrc
    {
        HRCReader::HRCReader(std::string &outputfolder) { output_folder = outputfolder; }

        HRCReader::~HRCReader() {}

        void HRCReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 21458)
                return;

            if (check_proba_crc(packet))
            {
                logger->error("HRC : Bad CRC!");
                return;
            }

            int img_tag = (packet.payload[0] - 1) << 16 | packet.payload[1] << 8 | packet.payload[2];
            int count_marker = packet.payload[18 - 6];

            if (hrc_images.count(img_tag) == 0)
            {
                logger->info("New HRC image with tag %d!", img_tag);
                hrc_images.insert({img_tag, std::make_shared<HRCImage>()});
            }

            std::shared_ptr<HRCImage> &curr = hrc_images[img_tag];

            if (count_marker > 65)
                return;

            // Convert into 10-bits values
            int pos = 21;
            for (int i = 0; i < 17152; i += 4)
            {
                if (count_marker <= 65)
                {
                    curr->tempChannelBuffer[count_marker * 17152 + i + 0] =
                        satdump::reverse16Bits((satdump::reverseBits(packet.payload[pos + 0]) << 2) | (satdump::reverseBits(packet.payload[pos + 1]) >> 6));
                    curr->tempChannelBuffer[count_marker * 17152 + i + 1] =
                        satdump::reverse16Bits(((satdump::reverseBits(packet.payload[pos + 1]) % 64) << 4) | (satdump::reverseBits(packet.payload[pos + 2]) >> 4));
                    curr->tempChannelBuffer[count_marker * 17152 + i + 2] =
                        satdump::reverse16Bits(((satdump::reverseBits(packet.payload[pos + 2]) % 16) << 6) | (satdump::reverseBits(packet.payload[pos + 3]) >> 2));
                    curr->tempChannelBuffer[count_marker * 17152 + i + 3] =
                        satdump::reverse16Bits(((satdump::reverseBits(packet.payload[pos + 3]) % 4) << 8) | satdump::reverseBits(packet.payload[pos + 4]));
                    pos += 5;
                }
            }
        }

        void HRCReader::save()
        {
            for (auto &imgh : hrc_images)
            {
                image::Image img = imgh.second->getImg();
                image::save_img(img, output_folder + "/HRC-" + std::to_string(imgh.first));
                image::normalize(img);
                image::equalize(img);
                image::save_img(img, output_folder + "/HRC-" + std::to_string(imgh.first) + "-EQU");
            }
        }
    } // namespace hrc
} // namespace proba