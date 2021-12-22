#include "mersi_banding.h"
#include <vector>
#include <algorithm>
#include <cstring>

namespace fengyun3
{
    namespace mersi
    {
        void banding_correct(image::Image<uint16_t> &imageo, int rowSize, float percent)
        {
            for (int ch = 0; ch < imageo.channels(); ch++)
            {
                unsigned short *image = &imageo[imageo.width() * imageo.height() * ch];

                int64_t *averages = new int64_t[rowSize];
                memset(averages, 0, rowSize * sizeof(int64_t));

                std::vector<std::vector<int>> calibValues;
                for (int i = 0; i < imageo.height(); i += rowSize)
                {
                    std::vector<int> coefs;
                    for (int y = 0; y < rowSize; y++)
                    {

                        std::vector<int> histogram;
                        for (int z = 0; z < imageo.width(); z++)
                            histogram.push_back(image[(i + y) * imageo.width() + z]);
                        std::sort(histogram.begin(), histogram.end());
                        //histogram.erase(std::unique(histogram.begin(), histogram.end()), histogram.end());

                        int value = histogram[int(histogram.size() * percent)];
                        coefs.push_back(value);
                    }
                    calibValues.push_back(coefs);
                }

                for (int z = 0; z < rowSize; z++)
                {
                    int64_t avg = 0;
                    for (std::vector<int> &coefs : calibValues)
                        avg += coefs[z];
                    avg /= calibValues.size();
                    averages[z] = avg;
                }

                int offset = averages[0];
                for (int z = 0; z < rowSize; z++)
                {
                    averages[z] -= offset;
                    //logger->info(averages[z]);
                }

                for (int i = 0; i < imageo.height(); i += rowSize)
                {
                    for (int y = 0; y < rowSize; y++)
                    {
                        for (int z = 0; z < imageo.width(); z++)
                        {
                            image[(i + y) * imageo.width() + z] = std::min<int>(65535, std::max<int>(0, image[(i + y) * imageo.width() + z] - averages[y]));
                        }
                    }
                }

                delete[] averages;
            }
        }
    };
};