#pragma once

#include "common/ccsds/ccsds.h"
#include <vector>
#include "common/image2/image.h"

namespace ldcm
{
    namespace tirs
    {
        class TIRSReader
        {
        private:
            std::vector<uint16_t> channels[3];
            uint16_t tirs_line[3898];

        public:
            TIRSReader();
            ~TIRSReader();

            void work(ccsds::CCSDSPacket &packet);
            image2::Image getChannel(int channel);

            int lines = 0;
        };
    }
}