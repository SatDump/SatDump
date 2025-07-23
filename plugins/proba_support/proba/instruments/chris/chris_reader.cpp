#include "chris_reader.h"
#include "../crc.h"
#include "common/repack.h"
#include "common/utils.h"
#include "image/io.h"
#include "logger.h"
#include "products/image_product.h"
#include "utils/binary.h"
#include "utils/stats.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>

#define ALL_MODE 2
#define WATER_MODE 3
#define LAND_MODE 3
#define CHLOROPHYL_MODE 3
#define LAND_ALL_MODE 100 // Never seen yet...

namespace proba
{
    namespace chris
    {
        CHRISReader::CHRISReader(std::string &outputfolder, satdump::products::DataSet &dataset) : dataset(dataset) { output_folder = outputfolder; }

        CHRISImageParser::CHRISImageParser()
        {
            img_buffer = std::vector<uint16_t>(748 * 7680 * absolute_max_cnt, 0);
            mode = 0;
            current_width = 12096;
            current_height = 748;
            max_value = 0; // 710;
            frame_count = 0;
        }

        CHRISImageParser::~CHRISImageParser() { img_buffer.clear(); }

        void CHRISImageParser::work(ccsds::CCSDSPacket &packet)
        {
            uint16_t count_marker = packet.payload[10] << 8 | packet.payload[11];
            int mode_marker = packet.payload[9] & 0x03;

            // Reverse bits... Recorder thing
            for (int i = 0; i < (int)packet.payload.size(); i++)
                packet.payload[i] = satdump::reverseBits(packet.payload[i]);

            // Check marker is in range
            if (count_marker > max_value - 1 && count_marker < absolute_max_cnt)
                max_value = count_marker + 1;

            // Repack to 12-bits
            bool bad = (packet.payload[16] & 0b1111111); //!(packet.payload[2] & 0b01000000);
            repackBytesTo12bits(&packet.payload[bad ? 18 : 16], packet.payload.size() - 16, words_tmp);

            // Convert into 12-bits values
            for (int i = 0; i < 7680; i += 1)
                if (count_marker < absolute_max_cnt)
                    img_buffer[count_marker * 7680 + (i + 0) + (bad ? 14 : 0)] = std::min<int>(65535, satdump::reverse16Bits(words_tmp[i]) << 1);

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
                mode = satdump::most_common(modeMarkers.begin(), modeMarkers.end(), 0);

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
            ret.raw = image::Image(img_buffer.data(), 16, current_width, final_height, 1);

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

                    image::save_img(chris_img.raw, dir_path + "/RAW");

                    satdump::products::ImageProduct chris_products;
                    chris_products.instrument_name = "chris";

                    for (int i = 0; i < (int)chris_img.channels.size(); i++)
                    {
                        image::Image ch = chris_img.channels[i];
                        ch.resize(ch.width() * 2, ch.height());
                        chris_products.images.push_back({i, "CHRIS-" + std::to_string(i + 1), std::to_string(i + 1), ch, 12});
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

                    image::save_img(chris_img.raw, dir_path + "/RAW");

                    satdump::products::ImageProduct chris_products;
                    chris_products.instrument_name = "chris";

                    for (int i = 0; i < (int)chris_img.channels.size(); i++)
                    {
                        image::Image ch = chris_img.channels[i];
                        chris_products.images.push_back({i, "CHRIS-" + std::to_string(i + 1), std::to_string(i + 1), ch, 12});
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

        image::Image CHRISFullFrameT::interleaveCHRIS(image::Image img1, image::Image img2)
        {
            image::Image img_final(img1.depth(), img1.width() * 2, img1.height(), 1);
            for (int i = 0; i < (int)img_final.width(); i += 2)
            {
                for (int y = 0; y < (int)img_final.height(); y++)
                {
                    img_final.set(y * img_final.width() + i + 0, img1.get(y * img1.width() + i / 2));
                    img_final.set(y * img_final.width() + i + 1, img2.get(y * img2.width() + i / 2));
                }
            }
            return img_final;
        }
    } // namespace chris
} // namespace proba