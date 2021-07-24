#pragma once

#include "common/ccsds/ccsds.h"
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
    //#include <common/aec/szlib.h>
}

namespace gk2a
{
    namespace lrit
    {
        class SegmentedLRITImageDecoder
        {
        private:
            int seg_count = 0;
            std::shared_ptr<bool> segments_done;
            int seg_height = 0, seg_width = 0;

        public:
            SegmentedLRITImageDecoder(int max_seg, int segment_width, int segment_height, std::string id);
            SegmentedLRITImageDecoder();
            ~SegmentedLRITImageDecoder();
            void pushSegment(uint8_t *data, int segc);
            bool isComplete();
            cimg_library::CImg<unsigned char> image;
            std::string image_id = "";
        };

        enum lrit_image_status
        {
            RECEIVING,
            SAVING,
            IDLE
        };

        class LRITDataDecoder
        {
        private:
            const std::string directory;

            bool file_in_progress;
            std::vector<uint8_t> lrit_data;

            bool is_encrypted;
            int key_index;
            std::map<int, uint64_t> decryption_keys;
            bool is_jpeg_compressed;
            std::string current_filename;
            std::vector<uint8_t> decompression_buffer;
            std::map<int, int> all_headers;
            SegmentedLRITImageDecoder segmentedDecoder;

            void processLRITHeader(ccsds::CCSDSPacket &pkt);
            void parseHeader();
            void processLRITData(ccsds::CCSDSPacket &pkt);
            void finalizeLRITData();

        public:
            LRITDataDecoder(std::string dir);
            ~LRITDataDecoder();

            void save();
            void work(ccsds::CCSDSPacket &packet);

            lrit_image_status imageStatus;
            int img_width, img_height;

        public:
            // UI Stuff
            bool hasToUpdate = false;
            unsigned int textureID = 0;
            uint32_t *textureBuffer;
        };
    } // namespace atms
} // namespace jpss