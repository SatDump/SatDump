#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "nlohmann/json.hpp"

namespace aws
{
    namespace navatt
    {
        class NavAttReader
        {
        private:
            int ephems_n = 0;
            nlohmann::json ephems;

        public:
            NavAttReader();
            ~NavAttReader();

            void work(ccsds::CCSDSPacket &packet);

            nlohmann::json getEphem();
        };
    }
}
