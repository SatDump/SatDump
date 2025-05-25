#pragma once

#include "common/ccsds/ccsds.h"
#include <vector>
#include "image/image.h"

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
            image::Image getImg1();
            image::Image getImg2();
        };
    } // namespace swap
} // namespace proba