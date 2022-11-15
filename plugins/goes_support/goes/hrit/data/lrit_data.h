#pragma once

#include "common/ccsds/ccsds.h"
#include <cmath>
#include "common/image/image.h"
#include <vector>
#include <string>
#include <map>
#include <memory>

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
            std::string filename;
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
    } // namespace atms
} // namespace jpss