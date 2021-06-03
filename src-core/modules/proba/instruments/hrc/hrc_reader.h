#pragma once

#include "common/ccsds/ccsds_1_0_proba/ccsds.h"

#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
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
            void work(ccsds::ccsds_1_0_proba::CCSDSPacket &packet);
        };
    } // namespace hrc
} // namespace proba