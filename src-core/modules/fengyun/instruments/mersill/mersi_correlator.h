#pragma once

#include <cstdint>
#include <vector>
#include "mersi_250m_reader.h"
#include "mersi_1000m_reader.h"

namespace fengyun
{
    namespace mersill
    {
        // Synchronizes channels for composites
        class MERSICorrelator
        {
        private:
            std::vector<uint8_t> scanBuffer[130];
            void processScan();
            MERSI250Reader reader1, reader2;
            MERSI1000Reader reader3, reader4, reader5, reader6, reader7;

        public:
            int complete;
            MERSICorrelator();
            void feedFrames(int marker, std::vector<uint8_t> &data);

        public:
            int m250Frames, m1000Frames;
            void makeImages();

            // Images
            cimg_library::CImg<unsigned short> image1;
            cimg_library::CImg<unsigned short> image2;
            cimg_library::CImg<unsigned short> image3;
            cimg_library::CImg<unsigned short> image4;
            cimg_library::CImg<unsigned short> image5;
            cimg_library::CImg<unsigned short> image6;
            cimg_library::CImg<unsigned short> image7;
        };
    } // namespace mersi2
} // namespace fengyun