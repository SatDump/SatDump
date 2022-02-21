#include "omps_limb_reader.h"
#include "common/ccsds/ccsds_time.h"
#include <algorithm>

namespace jpss
{
    namespace omps
    {
        OMPSLimbReader::OMPSLimbReader()
        {
            for (int i = 0; i < 135; i++)
                channels[i] = new unsigned short[10000 * 6];

            lines = 0;
            finalFrameVector = new uint8_t[1024 * 3000];

            rice_parameters.pixels_per_scanline = 256;
            rice_parameters.bits_per_pixel = 32;
            rice_parameters.pixels_per_block = 32;
            rice_parameters.options_mask = SZ_NN_OPTION_MASK | SZ_MSB_OPTION_MASK;
        }

        OMPSLimbReader::~OMPSLimbReader()
        {
            for (int i = 0; i < 135; i++)
                delete[] channels[i];
            delete[] finalFrameVector;
        }

        void OMPSLimbReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.header.sequence_flag == 1)
            {
                if (currentOMPSFrame.size() > 1000)
                {
                    if (packet.header.apid == 617) // Compressed, JPSS-1 and over
                    {
                        int end = currentOMPSFrame[141] == 0xEE ? currentOMPSFrame.size() - (143 + 6) - 1 : currentOMPSFrame.size() - (143 + 6);

                        std::vector<uint8_t> compressed_omps;
                        compressed_omps.insert(compressed_omps.end(), &currentOMPSFrame[143 + 6], &currentOMPSFrame[end]);

                        size_t output_size = 1024 * 3000;
                        int ret = SZ_BufftoBuffDecompress(finalFrameVector, &output_size, compressed_omps.data(), compressed_omps.size(), &rice_parameters);

                        if (ret == AEC_OK)
                        {
                            for (int channel = 0; channel < 135; channel++)
                            {
                                for (int i = 0; i < 6; i++)
                                {
                                    int pos = (64 + channel * 6 + i) * 4;
                                    uint32_t value = finalFrameVector[pos + 0] << 24 | finalFrameVector[pos + 1] << 16 | finalFrameVector[pos + 2] << 8 | finalFrameVector[pos + 3];
                                    channels[channel][lines * 6 + i] = std::min<uint32_t>(65535, value);
                                }
                            }

                            lines++;
                            timestamps.push_back(ccsds::parseCCSDSTimeFull(packet, -4383));
                        }
                    }
                    /*else if (packet.header.apid == 560) // Need to see what NPP requires
                    {
                        logger->info(currentOMPSFrame.size());
                        if (currentOMPSFrame.size() >= 29958) // Raw, Suomi NPP
                        {
                            std::memcpy(finalFrameVector, &currentOMPSFrame[143 + 6 + 16], 29958);
                            frames_out2.write((char *)finalFrameVector, 1024 * 3000);
                            for (int channel = 0; channel < 339; channel++)
                            {
                                for (int i = 0; i < 142; i++)
                                {
                                    int pos = (0 + channel * 142 + i) * 4;
                                    uint32_t value = finalFrameVector[pos + 0] << 24 | finalFrameVector[pos + 1] << 16 | finalFrameVector[pos + 2] << 8 | finalFrameVector[pos + 3];
                                    channels[channel][lines * 142 + i] = std::min<uint32_t>(65535, value >> 2);
                                }
                            }

                            lines++;
                        }
                    }*/
                }

                currentOMPSFrame.clear();
                currentOMPSFrame.insert(currentOMPSFrame.end(), packet.payload.begin(), packet.payload.end());
            }
            else if (packet.header.sequence_flag == 0 || packet.header.sequence_flag == 2)
            {
                currentOMPSFrame.insert(currentOMPSFrame.end(), packet.payload.begin(), packet.payload.end());
            }
        }

        image::Image<uint16_t> OMPSLimbReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel], 6, lines, 1);
        }
    } // namespace atms
} // namespace jpss