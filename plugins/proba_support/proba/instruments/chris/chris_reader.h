#pragma once

#include "common/ccsds/ccsds.h"

#include "common/image/image.h"
#include <string>
#include <map>
#include <memory>
#include "nlohmann/json.hpp"

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

        private:
            void writeHighResCompos(image::Image<uint16_t> &img);
            void writeAllCompos(image::Image<uint16_t> &img);
            std::string getModeName(int mode);

        public:
            CHRISImageParser(int &count, std::string &outputfolder);
            ~CHRISImageParser();
            void save();
            void work(ccsds::CCSDSPacket &packet, int &ch);

            nlohmann::json composites_all;
            nlohmann::json composites_low;
        };

        class CHRISReader
        {
        private:
            std::map<int, std::shared_ptr<CHRISImageParser>> imageParsers;
            std::string output_folder;

        public:
            CHRISReader(std::string &outputfolder);
            void work(ccsds::CCSDSPacket &packet);
            void save();

            int count;

            nlohmann::json composites_all;
            nlohmann::json composites_low;
        };
    } // namespace chris
} // namespace proba