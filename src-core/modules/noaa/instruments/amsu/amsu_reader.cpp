#include "amsu_reader.h"

namespace noaa
{
    namespace amsu
    {
        AMSUReader::AMSUReader()
        {
            for (int i = 0; i < 15; i++)
                channels[i] = new unsigned short[30 * 120];
        }

        AMSUReader::~AMSUReader()
        {
        }

        void AMSUReader::work(uint8_t *buffer)
        {
            std::vector<uint8_t> amsuA2words;
            std::vector<uint8_t> amsuA1words;

            for (int j = 0; j < 14; j += 2)
            {
                if ((buffer[j + 34] << 8) | (buffer[j + 35] != 1))
                {
                    amsuA2words.push_back(buffer[j + 34]);
                    amsuA2words.push_back(buffer[j + 35]);
                }
            }
            for (int j = 0; j < 26; j += 2)
            {
                if ((buffer[j + 8] << 8) | (buffer[j + 9] != 1))
                {
                    amsuA1words.push_back(buffer[j + 8]);
                    amsuA1words.push_back(buffer[j + 9]);
                }
            }

            std::vector<std::vector<uint8_t>> amsuA2Data = amsuA2Deframer.work(amsuA2words);
            std::vector<std::vector<uint8_t>> amsuA1Data = amsuA1Deframer.work(amsuA1words);

            for (std::vector<uint8_t> frame : amsuA2Data)
            {
                for (int i = 0; i < 240; i += 8)
                {
                    channels[0][30 * linesA2 + i / 8] = frame[i + 12] << 8 | frame[12 + i + 1];
                    channels[1][30 * linesA2 + i / 8] = frame[i + 12 + 2] << 8 | frame[12 + i + 1 + 2];
                }

                linesA2++;
            }

            for (std::vector<uint8_t> frame : amsuA1Data)
            {
                for (int i = 0; i < 1020; i += 34)
                {
                    for (int j = 0; j < 13; j++)
                    {
                        channels[j + 2][30 * linesA1 + i / 34] = (frame[i + 16 + 2 * j] << 8) | frame[16 + i + 2 * j + 1];
                    }
                }

                linesA1++;
            }
        }

        image::Image<uint16_t> AMSUReader::getChannel(int channel)
        {
            image::Image<uint16_t> img(channels[channel], 30, linesA1, 1);
            img.mirror(true, false);
            return img;
        }
    }
}