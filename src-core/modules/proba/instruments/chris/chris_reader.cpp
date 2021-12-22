#include "chris_reader.h"
#include <fstream>
#include <iostream>
#include <map>
#include "logger.h"

#define ALL_MODE 2
#define WATER_MODE 3
#define LAND_MODE 3
#define CHLOROPHYL_MODE 3
#define LAND_ALL_MODE 100 // Never seen yet...

#define WRITE_IMAGE_LOCAL(image, path)         \
    image.save_png(std::string(path).c_str()); \
    all_images.push_back(path);

template <class InputIt, class T = typename std::iterator_traits<InputIt>::value_type>
T most_common(InputIt begin, InputIt end)
{
    std::map<T, int> counts;
    for (InputIt it = begin; it != end; ++it)
    {
        if (counts.find(*it) != counts.end())
            ++counts[*it];
        else
            counts[*it] = 1;
    }
    return std::max_element(counts.begin(), counts.end(), [](const std::pair<T, int> &pair1, const std::pair<T, int> &pair2)
                            { return pair1.second < pair2.second; })
        ->first;
}

namespace proba
{
    namespace chris
    {
        CHRISReader::CHRISReader(std::string &outputfolder)
        {
            count = 0;
            output_folder = outputfolder;
        }

        CHRISImageParser::CHRISImageParser(int &count, std::string &outputfolder, std::vector<std::string> &a_images) : count_ref(count), all_images(a_images)
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

        void CHRISImageParser::work(ccsds::CCSDSPacket &packet, int & /*ch*/)
        {
            uint16_t count_marker = packet.payload[10] << 8 | packet.payload[11];
            int mode_marker = packet.payload[9] & 0x03;

            int tx_mode = (packet.payload[2] & 0b00000011) << 2 | packet.payload[3] >> 6;

            //logger->info("CH " << channel_marker );
            //logger->info("CNT " << count_marker );
            //logger->info("MODE " << mode_marker );
            //logger->info("TMD " << tx_mode );

            int posb = 16;

            if (mode_marker == ALL_MODE)
            {
                if (tx_mode == 8)
                    posb = 15;
                if (tx_mode == 0)
                    posb = 15;
                if (tx_mode == 200)
                    posb = 16;
                if (tx_mode == 72)
                    posb = 16;
                if (tx_mode == 200)
                    posb = 16;
                if (tx_mode == 136)
                    posb = 16;
                if (tx_mode == 192)
                    posb = 16;

                if (tx_mode == 128)
                    posb = 15;
                if (tx_mode == 64)
                    posb = 15;
            }
            else if (mode_marker == LAND_MODE)
            {
                if (tx_mode == 72)
                    posb = 16;
                if (tx_mode == 64)
                    posb = 16;
                if (tx_mode == 8)
                    posb = 16;
                if (tx_mode == 0)
                    posb = 16;
            }

            // Convert into 12-bits values
            for (int i = 0; i < 7680; i += 2)
            {
                if (count_marker <= max_value)
                {
                    uint16_t px1 = packet.payload[posb + 0] | ((packet.payload[posb + 1] & 0xF) << 8);
                    uint16_t px2 = (packet.payload[posb + 1] >> 4) | (packet.payload[posb + 2] << 4);
                    tempChannelBuffer[count_marker * 7680 + (i + 0)] = px1 << 4;
                    tempChannelBuffer[count_marker * 7680 + (i + 1)] = px2 << 4;
                    posb += 3;
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

            //logger->info("CH " << channel_marker );

            // Start new image
            if (imageParsers.find(channel_marker) == imageParsers.end())
            {
                logger->info("Found new CHRIS image! Marker " + std::to_string(channel_marker));
                imageParsers.insert(std::pair<int, std::shared_ptr<CHRISImageParser>>(channel_marker, std::make_shared<CHRISImageParser>(count, output_folder, all_images)));
            }

            imageParsers[channel_marker]->work(packet, channel_marker);
        }

        void CHRISImageParser::save()
        {
            if (frame_count != 0)
            {
                logger->info("Finished CHRIS image! Saving as CHRIS-" + std::to_string(count_ref) + ".png. Mode " + getModeName(mode));
                image::Image<uint16_t> img = image::Image<uint16_t>(tempChannelBuffer, current_width, current_height, 1);
                img.normalize();
                WRITE_IMAGE_LOCAL(img, output_folder + "/CHRIS-" + std::to_string(count_ref) + ".png");

                if (mode == CHLOROPHYL_MODE)
                    writeChlorophylCompos(img);
                if (mode == LAND_MODE)
                    writeLandCompos(img);
                if (mode == ALL_MODE)
                    writeAllCompos(img);

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

        void CHRISImageParser::writeChlorophylCompos(image::Image<uint16_t> &img)
        {
            logger->info("Writing chlorophyl mode RGB compositions...");
            image::Image<uint16_t> img9 = img;
            image::Image<uint16_t> img13 = img;
            image::Image<uint16_t> img16 = img;
            img9.crop(3077, 3077 + 375);
            img13.crop(4613, 4613 + 375);
            img16.crop(5765, 5765 + 375);

            image::Image<uint16_t> image16169(375, 748, 3);
            image16169.draw_image(0, img16);
            image16169.draw_image(1, img16);
            image16169.draw_image(2, img9);
            WRITE_IMAGE_LOCAL(image16169, output_folder + "/CHRIS-" + std::to_string(count_ref) + "-RGB16-16-9.png");

            image::Image<uint16_t> image13169(375, 748, 3);
            image13169.draw_image(0, img13);
            image13169.draw_image(1, img16);
            image13169.draw_image(2, img9);
            WRITE_IMAGE_LOCAL(image13169, output_folder + "/CHRIS-" + std::to_string(count_ref) + "-RGB13-16-9.png");
        }

        void CHRISImageParser::writeLandCompos(image::Image<uint16_t> &img)
        {
            logger->info("Writing water mode RGB compositions...");
            image::Image<uint16_t> imgs[19];
            for (int i = 0; i < 19; i++)
            {
                imgs[i] = img;
                imgs[i].crop(5 + i * 384, 5 + i * 384 + 375);
            }

            image::Image<uint16_t> image5170(375, 748, 3);
            image5170.draw_image(0, imgs[5]);
            image5170.draw_image(1, imgs[17]);
            image5170.draw_image(2, imgs[0]);
            WRITE_IMAGE_LOCAL(image5170, output_folder + "/CHRIS-" + std::to_string(count_ref) + "-RGB5-17-0.png");

            image::Image<uint16_t> image81410(375, 748, 3);
            image81410.draw_image(0, imgs[8]);
            image81410.draw_image(1, imgs[14]);
            image81410.draw_image(2, imgs[10]);
            WRITE_IMAGE_LOCAL(image81410, output_folder + "/CHRIS-" + std::to_string(count_ref) + "-RGB8-14-10.png");

            image::Image<uint16_t> image8130(375, 748, 3);
            image8130.draw_image(0, imgs[8]);
            image8130.draw_image(1, imgs[13]);
            image8130.draw_image(2, imgs[0]);
            WRITE_IMAGE_LOCAL(image8130, output_folder + "/CHRIS-" + std::to_string(count_ref) + "-RGB8-13-0.png");

            image::Image<uint16_t> image13169(375, 748, 3);
            image13169.draw_image(0, imgs[13]);
            image13169.draw_image(1, imgs[16]);
            image13169.draw_image(2, imgs[9]);
            WRITE_IMAGE_LOCAL(image13169, output_folder + "/CHRIS-" + std::to_string(count_ref) + "-RGB13-16-9.png");
        }

        void CHRISImageParser::writeAllCompos(image::Image<uint16_t> &img)
        {
            logger->info("Writing ALL mode RGB compositions...");
            image::Image<uint16_t> imgs[63];
            for (int i = 0; i < 63; i++)
            {
                imgs[i] = img;
                imgs[i].crop(4 + i * 192, 4 + i * 192 + 186);
                //WRITE_IMAGE_LOCAL(imgs[i], output_folder + "/CHRIS-" + std::to_string(count_ref) + "-CHANNEL-" + std::to_string(i + 1) + ".png");
            }

            image::Image<uint16_t> image8409(187, 374, 3);
            image8409.draw_image(0, imgs[8]);
            image8409.draw_image(1, imgs[40]);
            image8409.draw_image(2, imgs[9]);
            WRITE_IMAGE_LOCAL(image8409, output_folder + "/CHRIS-" + std::to_string(count_ref) + "-RGB8-40-9.png");

            image::Image<uint16_t> image204060(187, 374, 3);
            image204060.draw_image(0, imgs[20]);
            image204060.draw_image(1, imgs[40]);
            image204060.draw_image(2, imgs[60]);
            WRITE_IMAGE_LOCAL(image204060, output_folder + "/CHRIS-" + std::to_string(count_ref) + "-RGB20-40-60.png");

            image::Image<uint16_t> image305518(187, 374, 3);
            image305518.draw_image(0, imgs[30]);
            image305518.draw_image(1, imgs[55]);
            image305518.draw_image(2, imgs[18]);
            WRITE_IMAGE_LOCAL(image305518, output_folder + "/CHRIS-" + std::to_string(count_ref) + "-RGB30-55-18.png");
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