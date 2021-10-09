#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include <map>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include "libs/predict/predict.h"

namespace metop
{
    namespace sem
    {
        class SEMReader
        {
        private:
            predict_orbital_elements_t *metop_object;
            predict_position metop_orbit;
            cimg_library::CImg<unsigned char> map_image;

        public:
            SEMReader(int norad);
            ~SEMReader();

            void work(ccsds::CCSDSPacket &packet);
            cimg_library::CImg<unsigned char> getImage();
        };
    } // namespace modis
} // namespace eos