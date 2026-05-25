#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"
#include <map>
#include <vector>

#include <mutex>
#include <thread>

namespace proba
{
    namespace vegetation
    {
        class VegetationX
        {
        public:
            std::map<int, std::array<image::Image, 15>> img_segments;

        private:
            int segs_x;
            int seg_size_x;
            int seg_size_y;

            uint16_t *tmp_words;

            bool shouldStop = false;
            bool done = false;

            image::Image decompressBPE(std::vector<uint8_t> &pkt);

        public:
            VegetationX(int seg_size_x, int seg_size_y, int segs_x);
            ~VegetationX();

            void work(ccsds::CCSDSPacket &packet);

            image::Image getImg();
        };
    } // namespace vegetation
} // namespace proba