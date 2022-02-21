#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include <map>
#include "common/image/image.h"
#include "libs/predict/predict.h"

namespace jason3
{
    namespace lpt
    {
        class LPTReader
        {
        private:
            predict_orbital_elements_t *jason3_object;
            predict_position jason3_orbit;
            image::Image<uint8_t> *map_image;

            const int start_byte;
            const int channel_count;
            const int pkt_size;

        public:
            LPTReader(int start_byte, int channel_count, int pkt_size);
            ~LPTReader();

            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint8_t> getImage(int channel);
        };
    } // namespace modis
} // namespace eos