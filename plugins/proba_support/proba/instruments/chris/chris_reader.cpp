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
#include "../crc.h"

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
            output_folder = outputfolder;
        }

        CHRISImageParser::CHRISImageParser()
        {
            img_buffer = std::vector<uint16_t>(748 * 7680 * absolute_max_cnt, 0);
            mode = 0;
            current_width = 12096;
            current_height = 748;
            max_value = 0; // 710;
            frame_count = 0;
        }

        CHRISImageParser::~CHRISImageParser()
        {
            img_buffer.clear();
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

        void CHRISImageParser::work(ccsds::CCSDSPacket &packet)
        {
            uint16_t count_marker = packet.payload[10] << 8 | packet.payload[11];
            int mode_marker = packet.payload[9] & 0x03;

            // Reverse bits... Recorder thing
            for (int i = 0; i < (int)packet.payload.size(); i++)
                packet.payload[i] = reverseBits(packet.payload[i]);

            // Check marker is in range
            if (count_marker > max_value - 1 && count_marker < absolute_max_cnt)
                max_value = count_marker + 1;

            // Repack to 12-bits
            bool bad = (packet.payload[16] & 0b1111111); //!(packet.payload[2] & 0b01000000);
            repackBytesTo12bits(&packet.payload[bad ? 18 : 16], packet.payload.size() - 16, words_tmp);

            // Convert into 12-bits values
            for (int i = 0; i < 7680; i += 1)
                if (count_marker < absolute_max_cnt)
                    img_buffer[count_marker * 7680 + (i + 0) + (bad ? 14 : 0)] = std::min<int>(65535, reverse16Bits(words_tmp[i]) << 1);

            frame_count++;

            // Frame counter
            // if (count_marker == max_value)
            //{
            //    save();
            //    frame_count = 0;
            //    modeMarkers.clear();
            //}

            if ((count_marker > 50 && count_marker < 70) || (count_marker > 500 && count_marker < 520) || (count_marker > 700 && count_marker < 720))
            {
                mode = most_common(modeMarkers.begin(), modeMarkers.end());

                if (mode == WATER_MODE || mode == CHLOROPHYL_MODE || mode == LAND_MODE)
                {
                    current_width = 7296;
                    current_height = 748;
                    // max_value = 710;
                }
                else if (mode == ALL_MODE)
                {
                    current_width = 12096;
                    current_height = 374;
                    // max_value = 588;
                }
                else if (mode == LAND_ALL_MODE)
                {
                    current_width = 7680;
                    current_height = 374;
                    // max_value = 588;
                }
            }

            modeMarkers.push_back(mode_marker);
        }

        void CHRISReader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 11538)
                return;

            if (check_proba_crc(packet))
            {
                logger->error("CHRIS : Bad CRC!");
                return;
            }

            uint32_t img_full_id = packet.payload[0] << 24 | packet.payload[1] << 16 | packet.payload[2] << 8 | packet.payload[3];
            int img_tag = (packet.payload[0] - 1) << 16 | packet.payload[1] << 8 | packet.payload[2];

            // Start new image
            if (imageParsers.find(img_full_id) == imageParsers.end())
            {
                logger->info("Found new CHRIS image! Tag " + std::to_string(img_tag));
                imageParsers.insert(std::pair<uint32_t, std::shared_ptr<CHRISImageParser>>(img_full_id, std::make_shared<CHRISImageParser>()));
            }

            imageParsers[img_full_id]->work(packet);
        }

        CHRISImagesT CHRISImageParser::process()
        {
            int height_detected = (max_value * 7680) / current_width;
            int final_height = height_detected > current_height ? height_detected : current_height;
            logger->trace("CHRIS Image size : %dx%d", current_width, final_height);

            CHRISImagesT ret;
            ret.mode = mode;
            ret.raw = image::Image<uint16_t>(img_buffer.data(), current_width, final_height, 1);

            if (mode == ALL_MODE)
                for (int i = 0; i < 63; i++)
                    ret.channels.push_back(ret.raw.crop_to(4 + i * 192, 4 + i * 192 + 186));
            else if (mode == LAND_ALL_MODE)
                for (int i = 0; i < 20; i++)
                    ret.channels.push_back(ret.raw.crop_to(5 + i * 384, 5 + i * 384 + 375));
            else
                for (int i = 0; i < 19; i++)
                    ret.channels.push_back(ret.raw.crop_to(5 + i * 384, 5 + i * 384 + 375));

            return ret;
        }

        void CHRISReader::save()
        {
            logger->info("Saving CHRIS data! (if any)");

            std::map<int, CHRISFullFrameT> full_frames_wip;

            for (std::pair<const uint32_t, std::shared_ptr<CHRISImageParser>> &currentPair : imageParsers)
            {
                uint8_t tag[3];
                tag[0] = (currentPair.first >> 24) & 0xFF;
                tag[1] = (currentPair.first >> 16) & 0xFF;
                tag[2] = (currentPair.first >> 8) & 0xFF;
                int img_tag = (tag[0] - 1) << 16 | tag[1] << 8 | tag[2];
                bool is_2nd_half = currentPair.first & 0xFF;

                if (currentPair.second->frame_count > 0)
                {
                    CHRISImagesT chris_img = currentPair.second->process();

                    // Save Half alone
                    std::string image_name = std::to_string(img_tag) + "_Half_" + (is_2nd_half ? "2" : "1");

                    std::string dir_path = output_folder + "/CHRIS-" + image_name;

                    if (!std::filesystem::exists(dir_path))
                        std::filesystem::create_directories(dir_path);

                    chris_img.raw.save_img(dir_path + "/RAW");

                    satdump::ImageProducts chris_products;
                    chris_products.instrument_name = "chris";
                    chris_products.bit_depth = 12;
                    chris_products.has_timestamps = false;

                    for (int i = 0; i < (int)chris_img.channels.size(); i++)
                    {
                        image::Image<uint16_t> ch = chris_img.channels[i];
                        ch.resize(ch.width() * 2, ch.height());
                        chris_products.images.push_back({"CHRIS-" + std::to_string(i + 1), std::to_string(i + 1), ch});
                    }

                    chris_products.save(dir_path);
                    dataset.products_list.push_back("CHRIS/CHRIS-" + image_name);

                    // Also push the data to attempt recomposing later
                    if (full_frames_wip.find(img_tag) == full_frames_wip.end())
                        full_frames_wip.insert(std::pair<uint32_t, CHRISFullFrameT>(img_tag, CHRISFullFrameT()));

                    if (is_2nd_half)
                    {
                        full_frames_wip[img_tag].half2 = chris_img;
                        full_frames_wip[img_tag].has_half_2 = true;
                    }
                    else
                    {
                        full_frames_wip[img_tag].half1 = chris_img;
                        full_frames_wip[img_tag].has_half_1 = true;
                    }
                }
            }

            for (std::pair<const int, CHRISFullFrameT> &currentPair : full_frames_wip)
            {
                auto &frm = currentPair.second;

                if (frm.has_half_1 && frm.has_half_2)
                {
                    CHRISImagesT chris_img = frm.recompose();

                    // Save Full frame
                    std::string image_name = std::to_string(currentPair.first) + "_Full";

                    std::string dir_path = output_folder + "/CHRIS-" + image_name;

                    if (!std::filesystem::exists(dir_path))
                        std::filesystem::create_directories(dir_path);

                    chris_img.raw.save_img(dir_path + "/RAW");

                    satdump::ImageProducts chris_products;
                    chris_products.instrument_name = "chris";
                    chris_products.bit_depth = 12;
                    chris_products.has_timestamps = false;

                    for (int i = 0; i < (int)chris_img.channels.size(); i++)
                    {
                        image::Image<uint16_t> ch = chris_img.channels[i];
                        chris_products.images.push_back({"CHRIS-" + std::to_string(i + 1), std::to_string(i + 1), ch});
                    }

                    chris_products.save(dir_path);
                    dataset.products_list.push_back("CHRIS/CHRIS-" + image_name);
                }
            }
        }

        std::string getModeName(int mode)
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

        CHRISImagesT CHRISFullFrameT::recompose()
        {
            CHRISImagesT full;
            full.mode = half1.mode;
            full.raw = interleaveCHRIS(half1.raw, half2.raw);
            for (int i = 0; i < std::min<int>(half1.channels.size(), half2.channels.size()); i++)
                full.channels.push_back(interleaveCHRIS(half1.channels[i], half2.channels[i]));
            return full;
        }

        image::Image<uint16_t> CHRISFullFrameT::interleaveCHRIS(image::Image<uint16_t> img1, image::Image<uint16_t> img2)
        {
            image::Image<uint16_t> img_final(img1.width() * 2, img1.height(), 1);
            for (int i = 0; i < (int)img_final.width(); i += 2)
            {
                for (int y = 0; y < (int)img_final.height(); y++)
                {
                    img_final[y * img_final.width() + i + 0] = img1[y * img1.width() + i / 2];
                    img_final[y * img_final.width() + i + 1] = img2[y * img2.width() + i / 2];
                }
            }
            return img_final;
        }
    } // namespace chris
} // namespace proba