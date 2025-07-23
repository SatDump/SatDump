#pragma once

#include "common/ccsds/ccsds.h"

#include "image/image.h"
#include <string>
#include <map>
#include <memory>
#include "nlohmann/json.hpp"
#include "products/dataset.h"

namespace proba
{
    namespace chris
    {
        struct CHRISImagesT
        {
            int mode;
            image::Image raw;
            std::vector<image::Image> channels;
        };

        class CHRISImageParser
        {
        private:
            std::vector<uint16_t> img_buffer;
            std::vector<int> modeMarkers;
            int mode;
            int current_width, current_height, max_value;

            int absolute_max_cnt = 748 * 10;

            uint16_t words_tmp[100000];

        public:
            CHRISImageParser();
            ~CHRISImageParser();
            void work(ccsds::CCSDSPacket &packet);
            CHRISImagesT process();

            int frame_count = 0;
        };

        std::string getModeName(int mode);

        struct CHRISFullFrameT
        {
            bool has_half_1 = false;
            bool has_half_2 = false;
            CHRISImagesT half1;
            CHRISImagesT half2;

            CHRISImagesT recompose();
            image::Image interleaveCHRIS(image::Image img1, image::Image img2);
        };

        class CHRISReader
        {
        private:
            std::map<uint32_t, std::shared_ptr<CHRISImageParser>> imageParsers;
            std::string output_folder;
            satdump::products::DataSet &dataset;

        public:
            CHRISReader(std::string &outputfolder, satdump::products::DataSet &dataset);
            void work(ccsds::CCSDSPacket &packet);
            void save();
            int cnt() { return imageParsers.size(); }
        };
    } // namespace chris
} // namespace proba