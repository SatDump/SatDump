#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include <map>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include "libs/predict/predict.h"

namespace jason3
{
    namespace poseidon
    {
        class PoseidonReader
        {
        private:
            predict_orbital_elements_t *jason3_object;
            predict_position jason3_orbit;
            cimg_library::CImg<unsigned char> map_image_height, map_image_scatter;

        public:
            PoseidonReader();
            ~PoseidonReader();

            void work(ccsds::CCSDSPacket &packet);
            cimg_library::CImg<unsigned char> getImageHeight();
            cimg_library::CImg<unsigned char> getImageScatter();
        };
    } // namespace modis
} // namespace eos