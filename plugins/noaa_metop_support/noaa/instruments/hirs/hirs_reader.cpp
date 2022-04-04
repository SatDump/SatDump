#include "hirs_reader.h"
#include <cmath>
#include "common/repack.h"
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

                uint16_t words13bit[20] = {0};
                uint8_t tmp[32];
                shift_array_left(&HIRS_data[3], 32, 2, tmp);
                repackBytesTo13bits(tmp, 32, words13bit);

                imageBuffer[0][enct][line] = words13bit[0];
                imageBuffer[16][enct][line] = words13bit[1];
                imageBuffer[1][enct][line] = words13bit[2];
                imageBuffer[2][enct][line] = words13bit[3];
                imageBuffer[12][enct][line] = words13bit[4];
                imageBuffer[3][enct][line] = words13bit[5];
                imageBuffer[17][enct][line] = words13bit[6];
                imageBuffer[10][enct][line] = words13bit[7];
                imageBuffer[18][enct][line] = words13bit[8];
                imageBuffer[6][enct][line] = words13bit[9];
                imageBuffer[7][enct][line] = words13bit[10];
                imageBuffer[19][enct][line] = words13bit[11];
                imageBuffer[9][enct][line] = words13bit[12];
                imageBuffer[13][enct][line] = words13bit[13];
                imageBuffer[5][enct][line] = words13bit[14];
                imageBuffer[4][enct][line] = words13bit[15];
                imageBuffer[14][enct][line] = words13bit[16];
                imageBuffer[11][enct][line] = words13bit[17];
                imageBuffer[15][enct][line] = words13bit[18];
                imageBuffer[8][enct][line] = words13bit[19];

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