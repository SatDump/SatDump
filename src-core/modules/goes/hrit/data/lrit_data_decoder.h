#pragma once

#include "common/ccsds/ccsds_1_0_1024/ccsds.h"
#include <cmath>
#define cimg_use_png
#define cimg_display 0
#include "CImg.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
extern "C"
{
#include <common/aec/szlib.h>
}

namespace goes
{
    namespace hrit
    {
        class SegmentedLRITImageDecoder
        {
        private:
            int seg_count = 0;
            std::shared_ptr<bool> segments_done;
            int seg_height = 0, seg_width = 0;

        public:
            SegmentedLRITImageDecoder(int max_seg, int segment_width, int segment_height, uint16_t id);
            SegmentedLRITImageDecoder();
            ~SegmentedLRITImageDecoder();
            void pushSegment(uint8_t *data, int segc);
            bool isComplete();
            cimg_library::CImg<unsigned char> image;
            int image_id = -1;
        };

        class LRITDataDecoder
        {
        private:
            const std::string directory;

            bool file_in_progress;
            std::vector<uint8_t> lrit_data;

            bool is_rice_compressed;
            std::string current_filename;
            SZ_com_t rice_parameters;
            std::vector<uint8_t> decompression_buffer;
            std::map<int, int> all_headers;
            SegmentedLRITImageDecoder segmentedDecoder;

            void processLRITHeader(ccsds::ccsds_1_0_1024::CCSDSPacket &pkt);
            void parseHeader();
            void processLRITData(ccsds::ccsds_1_0_1024::CCSDSPacket &pkt);
            void finalizeLRITData();

        public:
            LRITDataDecoder(std::string dir);
            ~LRITDataDecoder();
            
            void save();
            void work(ccsds::ccsds_1_0_1024::CCSDSPacket &packet);
        };
    } // namespace atms
} // namespace jpss