#include "mats_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"

#include "logger.h"

#include <fstream>
#include <filesystem>

#include "common/utils.h"

#include "common/image2/jpeg12_utils.h"
#include "common/image2/io.h"

namespace mats
{
    namespace mats
    {
        MATSReader::MATSReader()
        {
        }

        MATSReader::~MATSReader()
        {
        }

        void MATSReader::work(ccsds::CCSDSPacket &pkt, std::string directory)
        {
            if (pkt.payload.size() < 13)
                return;

            int type_marker = pkt.payload[0];
            int channel_marker = pkt.payload[10];

            if (channel_marker >= 21 && channel_marker <= 27) // JPEG Operational payload
            {
                channel_marker -= 21;
                if (type_marker == 16)
                {
                    if (pkt.header.sequence_flag == 0b01 && pkt.payload.size() > 512)
                    {
                        wip_channel = channel_marker;
                        wip_payload.clear();
                        wip_payload.insert(wip_payload.end(), &pkt.payload[64], &pkt.payload[pkt.payload.size() - 2]);

                        // double time = ccsds::parseCCSDSTimeFullRaw(&pkt.payload[2], 1000);
                        // logger->critical(timestamp_to_string(time));
                    }
                    else if (pkt.header.sequence_flag == 0b00 && channel_marker == wip_channel)
                    {
                        wip_payload.insert(wip_payload.end(), &pkt.payload[11], &pkt.payload[pkt.payload.size() - 2]);
                    }
                    else if (pkt.header.sequence_flag == 0b10 && channel_marker == wip_channel)
                    {
                        wip_payload.insert(wip_payload.end(), &pkt.payload[11], &pkt.payload[pkt.payload.size() - 2]);

                        image2::Image img = image2::decompress_jpeg12(wip_payload.data(), wip_payload.size());

                        if (img.size() > 0)
                        {
                            if (wip_channel == 6)
                                process_nadir_imager(img);

                            {
                                std::string path = directory + "/Raw_Images/" + channel_names[channel_marker];
                                if (!std::filesystem::exists(path))
                                    std::filesystem::create_directories(path);
                                image2::save_img(img, path + "/" + std::to_string(img_cnts[channel_marker]++));
                            }
                        }
                    }
                }
                else
                {
                    // Uncompressed?
                }
            }
        }

        void MATSReader::process_nadir_imager(image2::Image &img)
        {
            for (int i = 0; i < 56; i++)
                nadir_image.push_back(img.get(0 + i));
            for (int i = 0; i < 56; i++)
                nadir_image.push_back(img.get(56 + i));
            nadir_lines += 2;
        }

        image2::Image MATSReader::getNadirImage()
        {
            return image2::Image(nadir_image.data(), 16, 56, nadir_lines, 1);
        }
    }
}