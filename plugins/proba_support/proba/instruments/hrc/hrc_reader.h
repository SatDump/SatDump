#pragma once

#include "common/ccsds/ccsds.h"

#include "common/image/image.h"
#include <string>

namespace proba
{
    namespace hrc
    {
        class HRCReader
        {
        private:
            unsigned short *tempChannelBuffer;

            int frame_count;
            std::string output_folder;

        public:
            HRCReader(std::string &outputfolder);
            ~HRCReader();

            int count;

            void save();
            void work(ccsds::CCSDSPacket &packet);
        };
    } // namespace hrc
} // namespace proba