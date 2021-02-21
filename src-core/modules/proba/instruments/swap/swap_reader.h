#pragma once

#include "modules/common/ccsds/ccsds_1_0_proba/ccsds.h"
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
            std::map<uint16_t, std::pair<int, std::pair<std::string, std::ofstream>>> currentOutFiles;
            std::string output_folder;

        public:
            std::vector<std::string> all_images;
            SWAPReader(std::string &outputfolder);
            void work(ccsds::ccsds_1_0_proba::CCSDSPacket &packet);
            void save();
        };
    } // namespace swap
} // namespace proba