#include "xfr.h"

#include <fstream>
#include <vector>
#include <sstream>
#include <cmath>

namespace image
{
    namespace xfr
    {
        XFR::XFR(float r1, float r2, float r3, float g1, float g2, float g3, float b1, float b2, float b3)
        {
            // Red values
            R[0] = r1;
            R[1] = r2;
            R[2] = 1.0f / (r3 / 100.0f);

            // Green values
            G[0] = g1;
            G[1] = g2;
            G[2] = 1.0f / (g3 / 100.0f);

            // Blue values
            B[0] = b1;
            B[1] = b2;
            B[2] = 1.0f / (b3 / 100.0f);

            // Generate LUTs
            for (int i = 0; i < 1024; i++)
            {
                if (i < R[0])
                {
                    red_lut[i] = 0.0f;
                }
                else if (i > R[1])
                {
                    red_lut[i] = 1023.0f;
                }
                else
                {
                    red_lut[i] = ((powf((i - R[0]) / (R[1] - R[0]), R[2]) * (R[1] - R[0])) + R[0]) * 1023.0f / R[1];
                }

                if (i < G[0])
                {
                    green_lut[i] = 0.0f;
                }
                else if (i > G[1])
                {
                    green_lut[i] = 1023.0f;
                }
                else
                {
                    green_lut[i] = ((powf((i - G[0]) / (G[1] - G[0]), G[2]) * (G[1] - G[0])) + G[0]) * 1023.0f / G[1];
                }

                if (i < B[0])
                {
                    blue_lut[i] = 0.0f;
                }
                else if (i > B[1])
                {
                    blue_lut[i] = 1023.0f;
                }
                else
                {
                    blue_lut[i] = ((powf((i - B[0]) / (B[1] - B[0]), B[2]) * (B[1] - B[0])) + B[0]) * 1023.0f / B[1];
                }
            }
        }

        void applyXFR(XFR &xfr, Image<uint16_t> &r, Image<uint16_t> &g, Image<uint16_t> &b)
        {
            // Apply XFR
            for (int i = 0; i < r.height() * r.width(); i++)
            {
                unsigned short &currentR = r.data()[i];
                unsigned short &currentG = g.data()[i];
                unsigned short &currentB = b.data()[i];

                currentR /= 60.0f;
                currentG /= 60.0f;
                currentB /= 60.0f;

                currentR = xfr.red_lut[currentR] * 60.0f;
                currentG = xfr.green_lut[currentG] * 60.0f;
                currentB = xfr.blue_lut[currentB] * 60.0f;
            }
        }

        void applyXFR(XFR &xfr, Image<uint16_t> &image)
        {
            // Apply XFR
            for (int i = 0; i < image.height() * image.width(); i++)
            {
                unsigned short &currentR = image.data()[image.height() * image.width() * 0 + i];
                unsigned short &currentG = image.data()[image.height() * image.width() * 1 + i];
                unsigned short &currentB = image.data()[image.height() * image.width() * 2 + i];

                currentR /= 60.0f;
                currentG /= 60.0f;
                currentB /= 60.0f;

                currentR = xfr.red_lut[currentR] * 60.0f;
                currentG = xfr.green_lut[currentG] * 60.0f;
                currentB = xfr.blue_lut[currentB] * 60.0f;
            }
        }

        XFR loadXFRFromFile(std::string path)
        {
            std::ifstream xfrFile(path);

            std::string currentLine;
            int lineCount = 0;

            float r1 = 0, r2 = 0, r3 = 0;
            float g1 = 0, g2 = 0, g3 = 0;
            float b1 = 0, b2 = 0, b3 = 0;

            while (std::getline(xfrFile, currentLine))
            {
                std::vector<std::string> values;
                std::string seg;
                std::stringstream line(currentLine);

                while (std::getline(line, seg, ' '))
                    values.push_back(seg);

                if (lineCount == 1)
                {
                    r1 = std::stof(values[1]);
                    r2 = std::stof(values[2]);
                    r3 = std::stof(values[3]);
                }
                else if (lineCount == 2)
                {
                    g1 = std::stof(values[1]);
                    g2 = std::stof(values[2]);
                    g3 = std::stof(values[3]);
                }
                else if (lineCount == 3)
                {
                    b1 = std::stof(values[1]);
                    b2 = std::stof(values[2]);
                    b3 = std::stof(values[3]);
                }

                lineCount++;
            }

            xfrFile.close();

            return XFR(r1, r2, r3, g1, g2, g3, b1, b2, b3);
        }
    }
}