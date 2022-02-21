#include "hirs_reader.h"
#include <cmath>
//#include "iostream"

namespace noaa
{
    namespace hirs
    {
        HIRSReader::HIRSReader()
        {
            for (int i = 0; i < 20; i++)
                channels[i] = new unsigned short[1000 * 56];
        }

        HIRSReader::~HIRSReader()
        {
            for (int i = 0; i < 20; i++)
                delete[] channels[i];
            //delete[] imageBuffer;
        }

        void HIRSReader::work(uint8_t *buffer)
        {
            uint8_t HIRS_data[36] = {};
            int pos = 0;
            for (int j : HIRSPositions)
            {
                HIRS_data[pos] = buffer[j];
                pos++;
                //std::cout<<pos<<std::endl;
            }

            uint16_t enct = ((HIRS_data[2] % (int)pow(2, 5)) << 1) | (HIRS_data[3] >> 7);
            //std::cout << "element number:" << enct << " encoder position:" << (unsigned int)HIRS_data[i][0] << std::endl;

            if (enct + 1 == (uint8_t)HIRS_data[0] && enct < 56)
            {
                int current = ((buffer[22] % (int)pow(2, 5)) << 1) | (buffer[23] >> 7);
                //std::cout<<last << ", " << enct << ", " << current <<std::endl;

                if (/*last + 1 > 54 && */ current == 55)
                {
                    //std::cout<<"yoss"<<std::endl;
                    line++;
                }
                //last = enct;

                imageBuffer[0][enct][line] = (HIRS_data[3] & 0b00111111) << 7 | HIRS_data[4] >> 1;
                imageBuffer[16][enct][line] = (HIRS_data[4] & 0b00000001) << 12 | HIRS_data[5] << 4 | (HIRS_data[6] & 0b11110000) >> 4;
                imageBuffer[1][enct][line] = (HIRS_data[6] & 0b00001111) << 9 | HIRS_data[7] << 1 | (HIRS_data[8] & 0b10000000) >> 7;
                imageBuffer[2][enct][line] = (HIRS_data[8] & 0b01111111) << 6 | (HIRS_data[9] & 0b11111100) >> 2;
                imageBuffer[12][enct][line] = (HIRS_data[9] & 0b00000011) << 11 | HIRS_data[10] << 3 | (HIRS_data[11] & 0b11100000) >> 5;
                imageBuffer[3][enct][line] = (HIRS_data[11] & 0b00011111) << 8 | HIRS_data[12];
                imageBuffer[17][enct][line] = HIRS_data[13] << 5 | (HIRS_data[14] & 0b11111000) >> 5;
                imageBuffer[10][enct][line] = (HIRS_data[14] & 0b00000111) << 10 | HIRS_data[15] << 2 | (HIRS_data[16] & 0b11000000) >> 6;
                imageBuffer[18][enct][line] = (HIRS_data[16] & 0b00111111) << 7 | HIRS_data[17] >> 1;
                imageBuffer[6][enct][line] = (HIRS_data[17] & 0b10000000) << 12 | HIRS_data[18] << 4 | (HIRS_data[19] & 0b11110000) >> 4;
                imageBuffer[7][enct][line] = (HIRS_data[19] & 0b00001111) << 9 | HIRS_data[20] << 1 | (HIRS_data[21] & 0b10000000) >> 7;
                imageBuffer[19][enct][line] = (HIRS_data[21] & 0b01111111) << 6 | (HIRS_data[22] & 0b11111100) >> 2;
                imageBuffer[9][enct][line] = (HIRS_data[22] & 0b00000011) << 11 | HIRS_data[23] << 3 | (HIRS_data[24] & 0b11100000) >> 5;
                imageBuffer[13][enct][line] = (HIRS_data[24] & 0b00011111) << 8 | HIRS_data[25];
                imageBuffer[5][enct][line] = HIRS_data[26] << 5 | (HIRS_data[27] & 0b11111000) >> 5;
                imageBuffer[4][enct][line] = (HIRS_data[27] & 0b00000111) << 10 | HIRS_data[28] << 2 | (HIRS_data[29] & 0b11000000) >> 6;
                imageBuffer[14][enct][line] = (HIRS_data[29] & 0b00111111) << 7 | HIRS_data[30] >> 1;
                imageBuffer[11][enct][line] = (HIRS_data[30] & 0b10000000) << 12 | HIRS_data[31] << 4 | (HIRS_data[32] & 0b11110000) >> 4;
                imageBuffer[15][enct][line] = (HIRS_data[32] & 0b00001111) << 9 | HIRS_data[33] << 1 | (HIRS_data[34] & 0b10000000) >> 7;
                imageBuffer[8][enct][line] = (HIRS_data[34] & 0b01111111) << 6 | (HIRS_data[35] & 0b11111100) >> 2;

                for (int i = 0; i < 20; i++)
                {
                    if ((imageBuffer[i][enct][line] >> 12) == 1)
                    {
                        imageBuffer[i][enct][line] = (imageBuffer[i][enct][line] & 0b0000111111111111) + 4095;
                    }
                    else
                    {
                        int buffer = 4096 - ((imageBuffer[i][enct][line] & 0b0000111111111111));
                        imageBuffer[i][enct][line] = abs(buffer);
                    }

                    channels[i][55 - enct + 56 * line] = imageBuffer[i][enct][line];
                }
            }
        }

        image::Image<uint16_t> HIRSReader::getChannel(int channel)
        {
            return image::Image<uint16_t>(channels[channel], 56, line, 1);
        }
    } // namespace hirs
} // namespace noaa