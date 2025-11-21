#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"
#include "nlohmann/json.hpp"

namespace metopsg
{
    namespace threemi
    {
        class ThreeMIReader
        {
        private:
            std::vector<uint8_t> img_vec;

        public:
            int img_n = 0;
            std::string directory;

        public:
            ThreeMIReader();
            ~ThreeMIReader();
            void work(ccsds::CCSDSPacket &packet);
        };
    } // namespace threemi
} // namespace metopsg