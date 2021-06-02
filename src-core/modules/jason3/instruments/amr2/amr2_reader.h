#pragma once

#include "common/ccsds/ccsds_1_0_jason/ccsds.h"
#include <cmath>
#include <map>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include "common/predict/predict.h"

namespace jason3
{
    namespace amr2
    {
        class AMR2Reader
        {
        private:
            predict_orbital_elements_t *jason3_object;
            predict_position jason3_orbit;
            cimg_library::CImg<unsigned char> map_image[3];
            unsigned short *imageBuffer[3];
            int lines;

        public:
            AMR2Reader();
            ~AMR2Reader();

            void work(ccsds::ccsds_1_0_jason::CCSDSPacket &packet);
            cimg_library::CImg<unsigned char> getImage(int channel);
            cimg_library::CImg<unsigned short> getImageNormal(int channel);
        };
    } // namespace modis
} // namespace eos