#pragma once

#include "common/ccsds/ccsds.h"
#include <map>
#include <array>
#include "common/image/image.h"

namespace fengyun3
{
    namespace erm
    {
        class ERMReader
        {
        private:
            std::map<double, std::array<unsigned short, 151>> imageData;

        public:
            ERMReader();
            ~ERMReader();
            int lines;
            std::vector<double> timestamps;
            void work(ccsds::CCSDSPacket &packet);
            image::Image<uint16_t> getChannel();
        };
    }
}