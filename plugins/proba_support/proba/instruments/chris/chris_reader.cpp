#include "chris_reader.h"
#include <fstream>
#include <iostream>
#include <map>
#include "logger.h"
#include "common/image/composite.h"
#include "common/utils.h"
#include "products/image_products.h"
#include <filesystem>
#include "common/repack.h"

#define ALL_MODE 2
#define WATER_MODE 3
#define LAND_MODE 3
#define CHLOROPHYL_MODE 3
#define LAND_ALL_MODE 100 // Never seen yet...

namespace proba
{
    namespace chris
    {
        CHRISReader::CHRISReader(std::string &outputfolder, satdump::ProductDataSet &dataset) : dataset(dataset)
        {
            count = 0;
            output_folder = outputfolder;
        }

        CHRISImageParser::CHRISImageParser(int &count, std::string &outputfolder, satdump::ProductDataSet &dataset) : count_ref(count), dataset(dataset)
        {
            tempChannelBuffer = new unsigned short[748 * 12096];
            mode = 0;
            current_width = 12096;
            current_height = 748;
            max_value = 710;
            frame_count = 0;
            output_folder = outputfolder;
        }

        CHRISImageParser::~CHRISImageParser()
        {
            delete[] tempChannelBuffer;
        }

        // std::ofstream chris_out("chris_out.bin");

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

        void CHRISImageParser::work(ccsds::CCSDSPacket &packet, int & /*ch*/)
        {
            uint16_t count_marker = packet.payload[10] << 8 | packet.payload[11];
            int mode_marker = packet.payload[9] & 0x03;

            //  logger->critical(packet.payload.size());

            for (int i = 0; i < (int)packet.payload.size(); i++)
                packet.payload[i] = reverseBits(packet.payload[i]);

            // chris_out.write((char *)packet.payload.data(), 11538);

            // int tx_mode = (packet.payload[2] & 0b00000011) << 2 | packet.payload[3] >> 6;

            // logger->info("CH " << channel_marker );
            // logger->info("CNT {:d}", count_marker);
            // logger->info("MODE " << mode_marker );
            // logger->info("TMD " << tx_mode );

            uint16_t out[100000];

            // uint32_t v = ((packet.payload[16] & 0b111111) << 3 | packet.payload[17] > 5);
            //  logger->info(v);

            bool bad = (packet.payload[16] & 0b1111111); //!(packet.payload[2] & 0b01000000);

            // logger->critical("BAD {:d}", bad);

            repackBytesTo12bits(&packet.payload[bad ? 18 : 16], packet.payload.size() - 16, out);

            // Convert into 12-bits values
            for (int i = 0; i < 7680; i += 1)
            {
                if (count_marker <= max_value)
                {
                    // uint16_t px1 = packet.payload[posb + 0] | ((packet.payload[posb + 1] & 0xF) << 8);
                    // uint16_t px2 = (packet.payload[posb + 1] >> 4) | (packet.payload[posb + 2] << 4);

                    tempChannelBuffer[count_marker * 7680 + (i + 0) + (bad ? 14 : 0)] = reverse16Bits(out[i]);
                    // tempChannelBuffer[count_marker * 7680 + (i + 1)] = px2 << 4;
                    // posb += 3;
                }
            }

            frame_count++;

            // Frame counter
            if (count_marker == max_value)
            {
                save();
                frame_count = 0;
                modeMarkers.clear();
            }

            if ((count_marker > 50 && count_marker < 70) || (count_marker > 500 && count_marker < 520) || (count_marker > 700 && count_marker < 720))
            {
                mode = most_common(modeMarkers.begin(), modeMarkers.end());

                if (mode == WATER_MODE || mode == CHLOROPHYL_MODE || mode == LAND_MODE)
                {
                    current_width = 7296;
                    current_height = 748;
                    max_value = 710;
                }
                else if (mode == ALL_MODE)
                {
                    current_width = 12096;
                    current_height = 374;
                    max_value = 588;
                }
                else if (mode == LAND_ALL_MODE)
                {
                    current_width = 7680;
                    current_height = 374;
                    max_value = 588;
                }
            }

            modeMarkers.push_back(mode_marker);
        }

        void CHRISReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 11538)
                return;

            int channel_marker = (packet.payload[8 - 6] % (int)pow(2, 3)) << 1 | packet.payload[9 - 6] >> 7;

            // logger->info("CH " << channel_marker );

            // Start new image
            if (imageParsers.find(channel_marker) == imageParsers.end())
            {
                logger->info("Found new CHRIS image! Marker " + std::to_string(channel_marker));
                imageParsers.insert(std::pair<int, std::shared_ptr<CHRISImageParser>>(channel_marker, std::make_shared<CHRISImageParser>(count, output_folder, dataset)));
            }

            imageParsers[channel_marker]->work(packet, channel_marker);
        }

        void CHRISImageParser::save()
        {
            if (frame_count != 0)
            {
                logger->info("Finished CHRIS image! Saving as CHRIS-" + std::to_string(count_ref) + ".png. Mode " + getModeName(mode));
                image::Image<uint16_t> img = image::Image<uint16_t>(tempChannelBuffer, current_width, current_height, 1);

                std::string dir_path = output_folder + "/CHRIS-" + std::to_string(count_ref);

                if (!std::filesystem::exists(dir_path))
                    std::filesystem::create_directories(dir_path);

                // img.normalize();
                img.save_png(dir_path + "/RAW.png");

                satdump::ImageProducts chris_products;
                chris_products.instrument_name = "chris";
                chris_products.bit_depth = 12;
                chris_products.has_timestamps = false;

                if (mode == ALL_MODE)
                {
                    for (int i = 0; i < 63; i++)
                    {
                        image::Image<uint16_t> ch = img;
                        ch.crop(4 + i * 192, 4 + i * 192 + 186);
                        ch.resize(ch.width() * 2, ch.height());
                        chris_products.images.push_back({"CHRIS-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), ch});
                    }
                }
                else
                {
                    for (int i = 0; i < 19; i++)
                    {
                        image::Image<uint16_t> ch = img;
                        ch.crop(5 + i * 384, 5 + i * 384 + 375);
                        ch.resize(ch.width() * 2, ch.height());
                        chris_products.images.push_back({"CHRIS-" + std::to_string(i + 1) + ".png", std::to_string(i + 1), ch});
                    }
                }

                chris_products.save(dir_path);
                dataset.products_list.push_back("CHRIS/CHRIS-" + std::to_string(count_ref));

                std::fill(&tempChannelBuffer[0], &tempChannelBuffer[748 * 12096], 0);
                count_ref++;
            }
        };

        void CHRISReader::save()
        {
            logger->info("Saving in-progress CHRIS data! (if any)");

            for (std::pair<int, std::shared_ptr<CHRISImageParser>> currentPair : imageParsers)
                currentPair.second->save();
        }

        std::string CHRISImageParser::getModeName(int mode)
        {
            if (mode == ALL_MODE)
                return "ALL";
            else if (mode == WATER_MODE)
                return "LAND/WATER/CHLOROPHYL";
            else if (mode == LAND_MODE)
                return "LAND/WATER/CHLOROPHYL";
            else if (mode == CHLOROPHYL_MODE)
                return "LAND/WATER/CHLOROPHYL";
            else if (mode == LAND_ALL_MODE)
                return "ALL-LAND";

            return "";
        }
    } // namespace chris
} // namespace proba