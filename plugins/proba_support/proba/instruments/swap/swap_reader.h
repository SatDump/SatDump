#pragma once

#include "common/ccsds/ccsds.h"
#include <fstream>
#include <map>

namespace proba
{
    namespace swap
    {
        class SWAPReader
        {
        private:
            struct WIP_Swap
            {
                std::string filename;
                int nsegs = 0;
                std::vector<uint8_t> payload_jpg;
                std::vector<uint8_t> payload_raw;
            };

            std::map<time_t, WIP_Swap> currentOuts;
            std::string output_folder;

        public:
            std::vector<std::string> all_images;
            SWAPReader(std::string &outputfolder);

            int count;

            void work(ccsds::CCSDSPacket &packet);
            void save();
        };
    } // namespace swap
} // namespace proba