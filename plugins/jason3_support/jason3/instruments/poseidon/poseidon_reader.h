#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include <map>
#include "common/image/image.h"
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
            image::Image<uint8_t> map_image_height, map_image_scatter;

        public:
            PoseidonReader();
            ~PoseidonReader();

            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint8_t> getImageHeight();
            image::Image<uint8_t> getImageScatter();
        };
    } // namespace modis
} // namespace eos