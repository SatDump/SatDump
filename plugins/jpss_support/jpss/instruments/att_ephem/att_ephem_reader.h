#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "nlohmann/json.hpp"

namespace jpss
{
    namespace att_ephem
    {
        class AttEphemReader
        {
        private:
            int ephems_n = 0;
            nlohmann::json ephems;

        public:
            AttEphemReader();
            ~AttEphemReader();

            void work(ccsds::CCSDSPacket &packet);

            nlohmann::json getEphem();
        };
    }
}
