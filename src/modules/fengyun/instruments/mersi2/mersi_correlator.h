#pragma once

#include <cstdint>
#include <vector>
#include "mersi_250m_reader.h"
#include "mersi_1000m_reader.h"

// Synchronizes channels for composites
class MERSICorrelator
{
private:
    int lastMarker;
    bool checking;
    std::vector<std::vector<uint8_t>> scanBuffer;
    void processScan();
    MERSI250Reader reader1, reader2, reader3, reader4, reader5, reader6;
    MERSI1000Reader reader7, reader8, reader9, reader10, reader11, reader12, reader13, reader14,
        reader15, reader16, reader17, reader18, reader19, reader20, reader21, reader22, reader23, reader24, reader25;

public:
    int complete, incomplete;
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
    cimg_library::CImg<unsigned short> image21;
    cimg_library::CImg<unsigned short> image22;
    cimg_library::CImg<unsigned short> image23;
    cimg_library::CImg<unsigned short> image24;
    cimg_library::CImg<unsigned short> image25;
};