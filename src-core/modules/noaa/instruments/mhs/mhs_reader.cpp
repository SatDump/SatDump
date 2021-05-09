#include "mhs_reader.h"

#include <iostream>

namespace noaa
{
    namespace mhs
    {
        MHSReader::MHSReader()
        {
            for (int i = 0; i < 5; i++)
                channels[i] = new unsigned short[90 * 400];
        }
        MHSReader::~MHSReader()
        {
            for (int i = 0; i < 5; i++)
                delete[] channels[i];
        }
        void MHSReader::work(uint8_t *buffer)
        {

            //get the MIU minor cycle count
            uint8_t MHSnum = buffer[7];

            //std::cout<<std::to_string(MHSnum)<<std::endl;

            //
            //second packet start (PKT 0)
            //

            //read the MHS Data from AIP frames (one of 3 in each AIP frame)
            if (MHSnum == 27)
            {
                std::fill_n(MHSWord, 643, 0);
                for (int j = 0; j < 18; j += 2)
                {
                    MHSWord[j / 2] = buffer[81 + j] << 8 | buffer[80 + j];
                }
            }
            else if (MHSnum > 27 && MHSnum < 53)
            {
                for (int j = 0; j < 50; j += 2)
                {
                    MHSWord[j / 2 + 1 + 25 * (MHSnum - 28)] = buffer[49 + j] << 8 | buffer[48 + j];
                }
            }
            else if (MHSnum == 53)
            {
                for (int j = 0; j < 18; j += 2)
                {
                    MHSWord[633 + j / 2] = buffer[49 + j] << 8 | buffer[48 + j];
                }

                for (int j = 0; j < 540; j += 6)
                {
                    for (int k = 0; k < 5; k++)
                        channels[k][line * 90 + 90 - j / 6 - 1] = MHSWord[j + k + 17];
                }
                line = line + 1;
            }

            //
            //second packet end (PKT 0)
            //

            //
            //third packet start (PKT 1)
            //

            //read the MHS Data from AIP frames (one of 3 in each AIP frame)
            if (MHSnum == 54)
            {
                std::fill_n(MHSWord, 643, 0);
                for (int j = 0; j < 36; j += 2)
                {
                    MHSWord[j / 2] = buffer[63 + j] << 8 | buffer[62 + j];
                }
            }
            else if (MHSnum > 54 && MHSnum < 80)
            {
                for (int j = 0; j < 50; j += 2)
                {
                    MHSWord[j / 2 + 1 + 25 * (MHSnum - 55)] = buffer[49 + j] << 8 | buffer[48 + j];
                }
            }
            if (MHSnum == 79)
            {

                for (int j = 0; j < 540; j += 6)
                {
                    //idk why I needed to add +49 there, but doesn't work without it
                    for (int k = 0; k < 5; k++)
                        channels[k][line * 90 + 90 - j / 6 - 1] = MHSWord[j + k + 8];
                }

                line = line + 1;
            }

            //
            //third packet end (PKT 1)
            //

            //
            //first packet start (PKT 2)
            //

            //read the MHS Data from AIP frames (one of 3 in each AIP frame)
            if (MHSnum == 0)
            {
                std::fill_n(MHSWord, 643, 0);
                MHSWord[0] = buffer[96] << 8 | buffer[97];
            }
            else if (MHSnum > 0 && MHSnum < 26)
            {
                for (int j = 0; j < 50; j += 2)
                {
                    MHSWord[j / 2 + 1 + 25 * (MHSnum - 1)] = buffer[49 + j] << 8 | buffer[48 + j];
                }
            }
            else if (MHSnum == 26)
            {
                for (int j = 0; j < 34; j += 2)
                {
                    MHSWord[625 + j / 2] = buffer[49 + j] << 8 | buffer[48 + j];
                }

                for (int j = 0; j < 90; j += 1)
                {
                    for (int k = 0; k < 5; k++)
                        channels[k][line * 90 + 90 - j - 1] = MHSWord[j * 6 + k + 25];
                }

                line = line + 1;
            }

            //
            //first packet end (PKT 2)
            //

            //AIP frame counter
            //line++;
        }

        cimg_library::CImg<unsigned short> MHSReader::getChannel(int channel)
        {
            return cimg_library::CImg<unsigned short>(channels[channel], 90, line + 1);
        }
    }
}