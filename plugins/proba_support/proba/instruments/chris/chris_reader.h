#pragma once

#include "common/ccsds/ccsds.h"

#include "common/image/image.h"
#include <string>
#include <map>
#include <memory>
#include "nlohmann/json.hpp"
#include "products/dataset.h"

namespace proba
{
    namespace chris
    {
        class CHRISImageParser
        {
        private:
            unsigned short *tempChannelBuffer;
            std::vector<int> modeMarkers;
            int &count_ref;
            int mode;
            int current_width, current_height, max_value;
            int frame_count;
            std::string output_folder;
            satdump::ProductDataSet &dataset;

        private:
            std::string getModeName(int mode);

        public:
            CHRISImageParser(int &count, std::string &outputfolder, satdump::ProductDataSet &dataset);
            ~CHRISImageParser();
            void save();
            void work(ccsds::CCSDSPacket &packet, int &ch);
        };

        class CHRISReader
        {
        private:
            std::map<int, std::shared_ptr<CHRISImageParser>> imageParsers;
            std::string output_folder;
            satdump::ProductDataSet &dataset;

        public:
            CHRISReader(std::string &outputfolder, satdump::ProductDataSet &dataset);
            void work(ccsds::CCSDSPacket &packet);
            void save();

            int count;
        };
    } // namespace chris
} // namespace proba