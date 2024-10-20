#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include <cstdint>
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
	    nlohmann::ordered_json telemetry;

	    uint8_t array[116];

        public:
            NavAttReader();
            ~NavAttReader();

            void work(ccsds::CCSDSPacket &packet);

	    int lines;
	    nlohmann::ordered_json dump_telemetry();
            nlohmann::json getEphem();
        };
    }
}
