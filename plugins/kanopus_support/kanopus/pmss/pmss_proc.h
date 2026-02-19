#pragma once

#include "pmss_segment.h"
#include <vector>

namespace kanopus
{
    namespace pmss
    {
        class PMSSProcessor
        {
        private:
            std::vector<std::vector<image::Image>> pss_img;
            std::vector<std::vector<image::Image>> mss_img;

        public:
            PMSSProcessor()
            {
                pss_img = std::vector<std::vector<image::Image>>(1, std::vector<image::Image>(6));
                mss_img = std::vector<std::vector<image::Image>>(1, std::vector<image::Image>(4));
            }

        public:
            void proc_PSS(image::Image img, int swath)
            {
                swath -= 1;

                // logger->critical("size %d %d", img.height(), img.width());

                if (swath == 0)
                    pss_img.push_back(std::vector<image::Image>(6));

                pss_img[pss_img.size() - 1][swath] = img;
            }

            void proc_MSS(image::Image img, int swath)
            {
                swath -= 7;

                // logger->critical("size %d %d", img.height(), img.width());

                if (swath == 0)
                    mss_img.push_back(std::vector<image::Image>(4));

                mss_img[mss_img.size() - 1][swath] = img;
            }

        public:
            image::Image get_pss_full()
            {
                image::Image fimg(8, 6 * 1920, (pss_img.size() + 2) * (495 - 32), 1);

                for (int y = 0; y < pss_img.size(); y++)
                {
                    for (int x = 0; x < 6; x++)
                    {
                        int offsets[] = {0, -19, 0, -19, 0, -19};
                        int offsetsX[] = {0, -75, -141, -213, -280, -350};

                        if (pss_img[y][5 - x].size())
                            fimg.draw_image(0, pss_img[y][5 - x], x * 1920 + offsetsX[x], (y + (x % 2 == 1 ? 2 : 0)) * (495 - 32) + offsets[x]);
                    }
                }

                return fimg;
            }

            image::Image get_mss_full(int s)
            {
                image::Image fimg(8, 1920, (mss_img.size() + 2) * (495 - 32), 1);

                for (int y = 0; y < mss_img.size(); y++)
                {
                    if (mss_img[y][s].size())
                        fimg.draw_image(0, mss_img[y][s], 0, y * (495 - 32));
                }

                return fimg;
            }
        };
    } // namespace pmss
} // namespace kanopus