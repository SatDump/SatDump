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
    //#include <common/aec/szlib.h>
}

namespace elektro
{
    namespace lrit
    {
        enum lrit_image_status
        {
            RECEIVING,
            SAVING,
            IDLE
        };

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

        class ELEKTRO221Composer
        {
        private:
            image::Image<uint8_t> ch2, ch1, compo221;
            time_t time2, time1;

            void generateCompo();

        public:
            ELEKTRO221Composer();
            ~ELEKTRO221Composer();

            bool hasData = false;

            std::string filename;

            void save(std::string directory);

            void push2(image::Image<uint8_t> img, time_t time);
            void push1(image::Image<uint8_t> img, time_t time);

        public:
            // UI Stuff
            lrit_image_status imageStatus;
            int img_width, img_height;
            bool hasToUpdate = false;
            unsigned int textureID = 0;
            uint32_t *textureBuffer;
        };

        class ELEKTRO321Composer
        {
        private:
            image::Image<uint8_t> ch3, ch2, ch1, compo321, compo231, compoNC;
            time_t time3, time2, time1;

            void generateCompo();

        public:
            ELEKTRO321Composer();
            ~ELEKTRO321Composer();

            bool hasData = false;

            std::string filename321, filename231, filenameNC;

            void save(std::string directory);

            void push3(image::Image<uint8_t> img, time_t time);
            void push2(image::Image<uint8_t> img, time_t time);
            void push1(image::Image<uint8_t> img, time_t time);

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