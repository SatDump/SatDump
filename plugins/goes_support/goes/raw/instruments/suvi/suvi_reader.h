#pragma once

#include "common/ccsds/ccsds.h"
#include <string>

namespace goes
{
    namespace instruments
    {
        namespace suvi
        {
            class SUVIReader
            {
            private:
                uint16_t *current_frame;

            public:
                SUVIReader();
                ~SUVIReader();
                void work(ccsds::CCSDSPacket &pkt);

                int img_cnt = 0;
                std::string directory;
            };
        }
    }
}