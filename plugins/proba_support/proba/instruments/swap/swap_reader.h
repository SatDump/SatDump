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
            int count;
            std::map<time_t, std::pair<int, std::pair<std::string, std::vector<uint8_t>>>> currentOuts;
            std::string output_folder;

        public:
            std::vector<std::string> all_images;
            SWAPReader(std::string &outputfolder);
            void work(ccsds::CCSDSPacket &packet);
            void save();
        };
    } // namespace swap
} // namespace proba