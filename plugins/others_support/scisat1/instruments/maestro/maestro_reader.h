#pragma once

#include "common/ccsds/ccsds.h"
#include <vector>
#include "common/image/image.h"

namespace scisat1
{
    namespace maestro
    {
        class MaestroReader
        {
        private:
            std::vector<uint16_t> img_data1;
            std::vector<uint16_t> img_data2;

        public:
            MaestroReader();
            ~MaestroReader();

            int lines_1;
            int lines_2;

            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getImg1();
            image::Image<uint16_t> getImg2();
        };
    } // namespace swap
} // namespace proba