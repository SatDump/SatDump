#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "common/image/image.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
extern "C"
{
#include <libs/aec/szlib.h>
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
            image::Image<uint8_t> image;
            int image_id = -1;
        };

        enum lrit_image_status
        {
            RECEIVING,
            SAVING,
            IDLE
        };

        class GOESRFalseColorComposer
        {
        private:
            image::Image<uint8_t> ch2_curve, fc_lut;
            image::Image<uint8_t> ch2, ch13, falsecolor;
            time_t time2, time13;

            void generateCompo();

        public:
            GOESRFalseColorComposer();
            ~GOESRFalseColorComposer();

            bool hasData = false;

            std::string filename;

            void save(std::string directory);

            void push2(image::Image<uint8_t> img, time_t time);
            void push13(image::Image<uint8_t> img, time_t time);

        public:
            // UI Stuff
            lrit_image_status imageStatus;
            int img_width, img_height;
            bool hasToUpdate = false;
            unsigned int textureID = 0;
            uint32_t *textureBuffer;
        };

        class LRITDataDecoder
        {
        private:
            const std::string directory;

            bool file_in_progress;
            std::vector<uint8_t> lrit_data;

            bool is_rice_compressed;
            bool is_goesn;
            std::string current_filename;
            SZ_com_t rice_parameters;
            std::vector<uint8_t> decompression_buffer;
            std::map<int, int> all_headers;
            SegmentedLRITImageDecoder segmentedDecoder;

            bool header_parsed = false;

            void processLRITHeader(ccsds::CCSDSPacket &pkt);
            void parseHeader();
            void processLRITData(ccsds::CCSDSPacket &pkt);
            void finalizeLRITData();

        public: // Other things
            std::shared_ptr<GOESRFalseColorComposer> goes_r_fc_composer_full_disk;
            std::shared_ptr<GOESRFalseColorComposer> goes_r_fc_composer_meso1;
            std::shared_ptr<GOESRFalseColorComposer> goes_r_fc_composer_meso2;

        public:
            LRITDataDecoder(std::string dir);
            ~LRITDataDecoder();

            bool write_images = true;
            bool write_emwin = true;
            bool write_messages = true;
            bool write_dcs = true;
            bool write_unknown = true;

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