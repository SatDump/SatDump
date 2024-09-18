#include "mersi_histmatch.h"
#include "common/image/histogram_utils.h"
#include "logger.h"
#include <cmath>

namespace fengyun3
{
    namespace mersi
    {
        void mersi_match_detector_histograms(image::Image &img, int ndet)
        {
            std::vector<std::vector<int>> pre_histograms_buffer(ndet);
            std::vector<std::vector<int>> all_histograms(ndet);
            std::vector<std::vector<int>> matching_tables(ndet);

            // Fill in buffers, also scale to 12-bits
            for (int i = 0; i < (int)img.height(); i++)
                for (int f = 0; f < (int)img.width(); f++)
                    pre_histograms_buffer[i % ndet].push_back(img.get(i * img.width() + f) >> 4);
            logger->trace("Filled buffers...");

            // Get histograms of all channels, then clear buffers
            for (int i = 0; i < ndet; i++)
                all_histograms[i] = image::histogram::get_histogram(pre_histograms_buffer[i], 4096);
            pre_histograms_buffer.clear();
            logger->trace("Made histograms...");

            // Equalize all histograms
            for (int i = 0; i < ndet; i++)
                all_histograms[i] = image::histogram::equalize_histogram(all_histograms[i]);
            logger->trace("Equalized histograms...");

            // Scale down to be faster
            for (int i = 0; i < ndet; i++)
                for (int y = 0; y < 4096; y++)
                    all_histograms[i][y] = round(all_histograms[i][y] * (float(4095) / float(img.size())));
            logger->trace("Scaled histograms...");

#pragma omp parallel for
            // Generate matching tables for all detectors
            for (int i = 0; i < ndet; i++)
            {
                if (i > 0)
                {
                    matching_tables[i] = image::histogram::make_hist_match_table(all_histograms[i], all_histograms[0], 100);
                }
                else
                {
                    matching_tables[i].resize(4096);
                    for (int ii = 0; ii < 4096; ii++)
                        matching_tables[i][ii] = ii;
                }
                logger->trace("Table %d done!", i + 1);
            }
            all_histograms.clear();
            logger->trace("Generated matching tables...");

            // Finally, apply on the image
            for (int f = 0; f < (int)img.width(); f++)
            {
                for (int i = 0; i < (int)img.height(); i++)
                {
                    int v = img.get(i * img.width() + f) >> 4;
                    v = matching_tables[i % ndet][v];
                    if (v < 0)
                        v = 0;
                    if (v > 4095)
                        v = 4095;
                    img.set(i * img.width() + f, v << 4);
                }
            }
            logger->trace("Applied...");
        }
    }
}