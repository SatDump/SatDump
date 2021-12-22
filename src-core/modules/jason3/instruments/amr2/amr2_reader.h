#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include <map>
#include "common/image/image.h"
#include "libs/predict/predict.h"

namespace jason3
{
    namespace amr2
    {
        class AMR2Reader
        {
        private:
            predict_orbital_elements_t *jason3_object;
            predict_position jason3_orbit;
            image::Image<uint8_t> map_image[3];
            unsigned short *imageBuffer[3];
            int lines;

        public:
            AMR2Reader();
            ~AMR2Reader();

            std::vector<double> timestamps;

            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint8_t> getImage(int channel);
            image::Image<uint16_t> getImageNormal(int channel);
        };
    } // namespace modis
} // namespace eos