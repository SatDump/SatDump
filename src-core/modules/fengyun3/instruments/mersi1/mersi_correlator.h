#pragma once

#include <cstdint>
#include <vector>
#include "mersi_250m_reader.h"
#include "mersi_1000m_reader.h"

namespace fengyun3
{
    namespace mersi1
    {
        // Synchronizes channels for composites
        class MERSICorrelator
        {
        private:
            std::vector<uint8_t> scanBuffer[350];
            void processScan();
            MERSI250Reader reader1, reader2, reader3, reader4, reader5;
            MERSI1000Reader reader6, reader7, reader8, reader9, reader10, reader11, reader12, reader13, reader14,
                reader15, reader16, reader17, reader18, reader19, reader20;

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
            cimg_library::CImg<unsigned short> image8;
            cimg_library::CImg<unsigned short> image9;
            cimg_library::CImg<unsigned short> image10;
            cimg_library::CImg<unsigned short> image11;
            cimg_library::CImg<unsigned short> image12;
            cimg_library::CImg<unsigned short> image13;
            cimg_library::CImg<unsigned short> image14;
            cimg_library::CImg<unsigned short> image15;
            cimg_library::CImg<unsigned short> image16;
            cimg_library::CImg<unsigned short> image17;
            cimg_library::CImg<unsigned short> image18;
            cimg_library::CImg<unsigned short> image19;
            cimg_library::CImg<unsigned short> image20;
        };
    } // namespace mersi1
} // namespace fengyun