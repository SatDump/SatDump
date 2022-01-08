#pragma once

#include <cstdint>
#include <vector>
#include "mersi_250m_reader.h"
#include "mersi_1000m_reader.h"

namespace fengyun3
{
    namespace mersi2
    {
        // Synchronizes channels for composites
        class MERSICorrelator
        {
        private:
            std::vector<uint8_t> scanBuffer[430];
            void processScan();
            MERSI250Reader reader1, reader2, reader3, reader4, reader5, reader6;
            MERSI1000Reader reader7, reader8, reader9, reader10, reader11, reader12, reader13, reader14,
                reader15, reader16, reader17, reader18, reader19, reader20, reader21, reader22, reader23, reader24, reader25;

        public:
            int complete;
            MERSICorrelator();
            void feedFrames(int marker, std::vector<uint8_t> &data);

        public:
            int m250Frames, m1000Frames;
            void makeImages();

            // Images
            image::Image<uint16_t> image1;
            image::Image<uint16_t> image2;
            image::Image<uint16_t> image3;
            image::Image<uint16_t> image4;
            image::Image<uint16_t> image5;
            image::Image<uint16_t> image6;
            image::Image<uint16_t> image7;
            image::Image<uint16_t> image8;
            image::Image<uint16_t> image9;
            image::Image<uint16_t> image10;
            image::Image<uint16_t> image11;
            image::Image<uint16_t> image12;
            image::Image<uint16_t> image13;
            image::Image<uint16_t> image14;
            image::Image<uint16_t> image15;
            image::Image<uint16_t> image16;
            image::Image<uint16_t> image17;
            image::Image<uint16_t> image18;
            image::Image<uint16_t> image19;
            image::Image<uint16_t> image20;
            image::Image<uint16_t> image21;
            image::Image<uint16_t> image22;
            image::Image<uint16_t> image23;
            image::Image<uint16_t> image24;
            image::Image<uint16_t> image25;
        };
    } // namespace mersi2
} // namespace fengyun