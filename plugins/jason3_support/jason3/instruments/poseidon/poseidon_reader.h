#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include <map>
// #include "common/image/image.h"
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

            void work(ccsds::CCSDSPacket &packet);
        };
    } // namespace modis
} // namespace eos