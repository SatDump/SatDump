#include "bowtie.h"
#include <algorithm>

namespace bowtie
{
    cimg_library::CImg<unsigned short> correctGenericBowTie(cimg_library::CImg<unsigned short> &inputImage, const int channelCount, const long scanHeight, const float alpha, const float beta)
    {
        // Compute everything we'll need
        const long height = inputImage.height();
        const long width = inputImage.width();
        const long scanCount = height / scanHeight;
        const long halfWidth = width / 2;

        // Create our output image
        cimg_library::CImg<unsigned short> outputImage = cimg_library::CImg<unsigned short>(width, height, 1, channelCount);

        // Reserve our buffers
        unsigned short *scan_buffer_input = new unsigned short[height * width];
        unsigned short *scan_buffer_output = new unsigned short[height * width];
        unsigned short *col_buffer_input = new unsigned short[scanHeight];
        unsigned short *col_buffer_output = new unsigned short[scanHeight];

        for (int channel = 0; channel < channelCount; channel++)
        {
            for (int scanNumber = 0; scanNumber < scanCount; scanNumber++)
            {
                //std::cout << "Processing scan " << scanNumber << std::endl;

                // Load out input buffer
                for (int lineNumber = 0; lineNumber < scanHeight; lineNumber++)
                {
                    for (int pixelNumber = 0; pixelNumber < width; pixelNumber++)
                    {
                        scan_buffer_input[lineNumber * width + pixelNumber] = inputImage[(channel * width * height) + ((scanNumber * scanHeight) + lineNumber) * width + pixelNumber];
                    }
                }

                for (int rowNumber = 0; rowNumber < width; rowNumber++)
                {
                    // Load our column
                    for (int lineNumber = 0; lineNumber < scanHeight; lineNumber++)
                    {
                        col_buffer_input[lineNumber] = scan_buffer_input[lineNumber * width + rowNumber];
                    }

                    int centerPixelCounts = int((((halfWidth - abs(rowNumber - halfWidth)) / (float)halfWidth) * alpha + beta) * scanHeight);
                    centerPixelCounts = std::min<int>(centerPixelCounts, scanHeight);

                    int paddingPixels = (scanHeight - centerPixelCounts) / 2;

                    //std::cout << pixelsOffCenter << " " << centerPixelCounts << " " << paddingPixels << std::endl;

                    for (int i = 0; i < scanHeight; i++)
                    {
                        col_buffer_output[i] = col_buffer_input[paddingPixels + int(((float)i / (float)scanHeight) * centerPixelCounts)];
                    }

                    // Offload our columm
                    for (int lineNumber = 0; lineNumber < scanHeight; lineNumber++)
                    {
                        scan_buffer_output[lineNumber * width + rowNumber] = col_buffer_output[lineNumber];
                    }
                }

                // Offload out output buffer
                for (int lineNumber = 0; lineNumber < scanHeight; lineNumber++)
                {
                    for (int pixelNumber = 0; pixelNumber < width; pixelNumber++)
                    {
                        outputImage[(channel * width * height) + ((scanNumber * scanHeight) + lineNumber) * width + pixelNumber] = scan_buffer_output[lineNumber * width + pixelNumber];
                    }
                }
            }
        }

        delete[] scan_buffer_input;
        delete[] scan_buffer_output;
        delete[] col_buffer_input;
        delete[] col_buffer_output;

        return outputImage;
    }
}