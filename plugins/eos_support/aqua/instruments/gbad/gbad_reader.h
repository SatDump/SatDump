#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "nlohmann/json.hpp"

namespace aqua
{
    namespace gbad
    {
        class GBADReader
        {
        private:
            int ephems_n = 0;
            nlohmann::json ephems;

        public:
            GBADReader();
            ~GBADReader();

            void work(ccsds::CCSDSPacket &packet);

            nlohmann::json getEphem();
        };
    }
}
