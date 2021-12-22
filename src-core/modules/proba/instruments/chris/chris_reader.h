#pragma once

#include "common/ccsds/ccsds.h"

#include "common/image/image.h"
#include <string>
#include <map>
#include <memory>

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

            std::vector<std::string> &all_images;

        private:
            void writeChlorophylCompos(image::Image<uint16_t> &img);
            void writeLandCompos(image::Image<uint16_t> &img);
            void writeAllCompos(image::Image<uint16_t> &img);
            std::string getModeName(int mode);

        public:
            CHRISImageParser(int &count, std::string &outputfolder, std::vector<std::string> &a_images);
            ~CHRISImageParser();
            void save();
            void work(ccsds::CCSDSPacket &packet, int &ch);
        };

        class CHRISReader
        {
        private:
            int count;
            std::map<int, std::shared_ptr<CHRISImageParser>> imageParsers;
            std::string output_folder;

        public:
            std::vector<std::string> all_images;
            CHRISReader(std::string &outputfolder);
            void work(ccsds::CCSDSPacket &packet);
            void save();
        };
    } // namespace chris
} // namespace proba