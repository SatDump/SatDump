#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "common/image/image.h"
#include <vector>
#include <string>
#include <map>
#include <memory>

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
            image::Image<uint8_t> image;
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

            bool header_parsed = false;

            void processLRITHeader(ccsds::CCSDSPacket &pkt);
            void parseHeader();
            void processLRITData(ccsds::CCSDSPacket &pkt);
            void finalizeLRITData();

        public:
            LRITDataDecoder(std::string dir);
            ~LRITDataDecoder();

            bool write_images = true;
            bool write_additional = true;
            bool write_unknown = true;

            void save();
            void work(ccsds::CCSDSPacket &packet);

            std::map<std::string, lrit_image_status> imageStatus;
            std::map<std::string, int> img_widths, img_heights;

            std::map<std::string, SegmentedLRITImageDecoder> segmentedDecoders;

        public:
            // UI Stuff
            std::map<std::string, bool> hasToUpdates;
            std::map<std::string, unsigned int> textureIDs;
            std::map<std::string, uint32_t *> textureBuffers;
        };
    } // namespace atms
} // namespace jpss