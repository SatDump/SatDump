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
            int count;
            int frame_count;
            std::string output_folder;

        public:
            std::vector<std::string> all_images;
            HRCReader(std::string &outputfolder);
            ~HRCReader();
            void save();
            void work(ccsds::CCSDSPacket &packet);
        };
    } // namespace hrc
} // namespace proba