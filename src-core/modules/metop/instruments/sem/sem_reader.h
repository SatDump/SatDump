#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include <map>
#include "common/image/image.h"
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
            image::Image<uint8_t> map_image;

        public:
            SEMReader(int norad);
            ~SEMReader();

            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint8_t> getImage();
        };
    } // namespace modis
} // namespace eos