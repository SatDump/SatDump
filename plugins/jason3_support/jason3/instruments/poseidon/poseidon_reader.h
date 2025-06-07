#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include <map>
// #include "image/image.h"
#include "libs/predict/predict.h"

namespace jason3
{
    namespace poseidon
    {
        class PoseidonReader
        {
            // private:
        public:
            PoseidonReader();
            ~PoseidonReader();

            int frames;
            std::vector<double> timestamps;

            // TODOREWORK Proper radar as 3D limb
            std::vector<double> data_height;
            std::vector<double> data_scatter;

            std::vector<double> data_unknown;

            void work(ccsds::CCSDSPacket &packet);
        };
    } // namespace modis
} // namespace eos