#include "sd_imager_reader.h"
#include "logger.h"
#include "common/utils.h"
#include <filesystem>
#include <cstring>
#include "common/image/io.h"
#include "common/image/image_processing.h"

#define IMG_WIDTH 40000

namespace goes
{
    namespace sd
    {
        SDImagerReader::SDImagerReader()
        {
            memset(last_status, 0, FULL_BUF_SZ * sizeof(int));
        }

        void SDImagerReader::work(uint16_t *words)
        {
            int type = words[1];

            memmove(last_status, &last_status[1], (FULL_BUF_SZ - 1) * sizeof(int));
            last_status[FULL_BUF_SZ - 1] = type;

            int last_types = most_common(&last_status[0], &last_status[FULL_BUF_SZ], 0);

            if (last_types == 16 && images_lines > 10)
            {
                logger->critical("END OF FRAME");
                images_lines = 0;
                should_save = true;
            }
            else if (last_types == 16)
            {
                images_lines = 0;
            }

            if (type == 21)
            {
                image_vis.resize(size_t(lines + 1) * 8 * IMG_WIDTH);
                image_ir1.resize(size_t(lines + 1) * 2 * IMG_WIDTH);
                image_ir2.resize(size_t(lines + 1) * 2 * IMG_WIDTH);
                image_ir3.resize(size_t(lines + 1) * 2 * IMG_WIDTH);
                image_ir4.resize(size_t(lines + 1) * 2 * IMG_WIDTH);

                bool is_shifted = ((words[3] >> 6) & 1);

                int x = is_shifted ? ((20917 - 70 + 12 - 3 - 8) / 4) : 0;

                for (int y = 0; y < (int)wip_scanline.size() / 48; y++)
                {
                    uint16_t *block = &wip_scanline[y * 48];

                    if (!is_shifted)
                    {
                        for (int b = 0; b < 4; b++)
                        {
                            image_vis[size_t(lines * 8 + 0) * IMG_WIDTH + (x * 4 + b)] = block[b * 12 + 9] << 6;
                            image_vis[size_t(lines * 8 + 1) * IMG_WIDTH + (x * 4 + b)] = block[b * 12 + 8] << 6;
                            image_vis[size_t(lines * 8 + 2) * IMG_WIDTH + (x * 4 + b)] = block[b * 12 + 7] << 6;
                            image_vis[size_t(lines * 8 + 3) * IMG_WIDTH + (x * 4 + b)] = block[b * 12 + 6] << 6;
                            image_vis[size_t(lines * 8 + 4) * IMG_WIDTH + (x * 4 + b)] = block[b * 12 + 5] << 6;
                            image_vis[size_t(lines * 8 + 5) * IMG_WIDTH + (x * 4 + b)] = block[b * 12 + 4] << 6;
                            image_vis[size_t(lines * 8 + 6) * IMG_WIDTH + (x * 4 + b)] = block[b * 12 + 3] << 6;
                            image_vis[size_t(lines * 8 + 7) * IMG_WIDTH + (x * 4 + b)] = block[b * 12 + 2] << 6;
                        }

                        image_ir1[size_t(lines * 2 + 0) * IMG_WIDTH + x] = 65535 - (block[0 * 12 + 10] << 6);
                        image_ir1[size_t(lines * 2 + 1) * IMG_WIDTH + x] = 65535 - (block[0 * 12 + 11] << 6);

                        image_ir2[size_t(lines * 2 + 0) * IMG_WIDTH + x] = 65535 - (block[1 * 12 + 10] << 6);
                        image_ir2[size_t(lines * 2 + 1) * IMG_WIDTH + x] = 65535 - (block[1 * 12 + 11] << 6);

                        image_ir3[size_t(lines * 2 + 0) * IMG_WIDTH + x] = 65535 - (block[2 * 12 + 10] << 6);
                        image_ir3[size_t(lines * 2 + 1) * IMG_WIDTH + x] = 65535 - (block[2 * 12 + 11] << 6);

                        image_ir4[size_t(lines * 2 + 0) * IMG_WIDTH + x] = 65535 - (block[3 * 12 + 10] << 6);
                        image_ir4[size_t(lines * 2 + 1) * IMG_WIDTH + x] = 65535 - (block[3 * 12 + 10] << 6);
                    }
                    else
                    {
                        for (int b = 0; b < 4; b++)
                        {
                            image_vis[size_t(lines * 8 + 0) * IMG_WIDTH + (x * 4 + 3 - b)] = block[b * 12 + 9] << 6;
                            image_vis[size_t(lines * 8 + 1) * IMG_WIDTH + (x * 4 + 3 - b)] = block[b * 12 + 8] << 6;
                            image_vis[size_t(lines * 8 + 2) * IMG_WIDTH + (x * 4 + 3 - b)] = block[b * 12 + 7] << 6;
                            image_vis[size_t(lines * 8 + 3) * IMG_WIDTH + (x * 4 + 3 - b)] = block[b * 12 + 6] << 6;
                            image_vis[size_t(lines * 8 + 4) * IMG_WIDTH + (x * 4 + 3 - b)] = block[b * 12 + 5] << 6;
                            image_vis[size_t(lines * 8 + 5) * IMG_WIDTH + (x * 4 + 3 - b)] = block[b * 12 + 4] << 6;
                            image_vis[size_t(lines * 8 + 6) * IMG_WIDTH + (x * 4 + 3 - b)] = block[b * 12 + 3] << 6;
                            image_vis[size_t(lines * 8 + 7) * IMG_WIDTH + (x * 4 + 3 - b)] = block[b * 12 + 2] << 6;
                        }

                        image_ir1[size_t(lines * 2 + 0) * IMG_WIDTH + x] = 65535 - (block[0 * 12 + 10] << 6);
                        image_ir1[size_t(lines * 2 + 1) * IMG_WIDTH + x] = 65535 - (block[0 * 12 + 11] << 6);

                        image_ir2[size_t(lines * 2 + 0) * IMG_WIDTH + x] = 65535 - (block[1 * 12 + 10] << 6);
                        image_ir2[size_t(lines * 2 + 1) * IMG_WIDTH + x] = 65535 - (block[1 * 12 + 11] << 6);

                        image_ir3[size_t(lines * 2 + 0) * IMG_WIDTH + x] = 65535 - (block[2 * 12 + 10] << 6);
                        image_ir3[size_t(lines * 2 + 1) * IMG_WIDTH + x] = 65535 - (block[2 * 12 + 11] << 6);

                        image_ir4[size_t(lines * 2 + 0) * IMG_WIDTH + x] = 65535 - (block[3 * 12 + 10] << 6);
                        image_ir4[size_t(lines * 2 + 1) * IMG_WIDTH + x] = 65535 - (block[3 * 12 + 10] << 6);
                    }

                    if (is_shifted)
                        x--;
                    else
                        x++;

                    if (x < 0)
                        x = 0;
                }

                lines++;
                images_lines++;

                wip_scanline.clear();

                logger->info("Lines %d", lines);
            }

            wip_scanline.insert(wip_scanline.end(), &words[0], &words[48]);
        }

        image::Image SDImagerReader::getChannel(int c)
        {
            image::Image img;

            if (c == 0)
                img = image::Image(image_vis.data(), 16, IMG_WIDTH, lines * 8, 1);
            else if (c == 1)
                img = image::Image(image_ir1.data(), 16, IMG_WIDTH, lines * 2, 1);
            else if (c == 2)
                img = image::Image(image_ir2.data(), 16, IMG_WIDTH, lines * 2, 1);
            else if (c == 3)
                img = image::Image(image_ir3.data(), 16, IMG_WIDTH, lines * 2, 1);
            else if (c == 4)
                img = image::Image(image_ir4.data(), 16, IMG_WIDTH, lines * 2, 1);

            if (c == 0)
                img.crop(0, 21072);
            else
                img.crop(0, 21072 / 4);

            image::median_blur(img);

            img.resize_bilinear(img.width(), img.height() * 1.75);

            // img.mirror(true, false);

            return img;
        }

        void SDImagerReader::try_save(std::string directory, bool force)
        {
            if (should_save || force)
            {
                nsaved++;

                std::string dir = directory + "/" + std::to_string(nsaved) + "/";

                if (!std::filesystem::exists(dir))
                    std::filesystem::create_directories(dir);

                auto img = getChannel(0);
                image::save_img(img, dir + "/VIS", false);
                img = getChannel(1);
                image::save_img(img, dir + "/IR1", false);
                img = getChannel(2);
                image::save_img(img, dir + "/IR2", false);
                img = getChannel(3);
                image::save_img(img, dir + "/IR3", false);
                img = getChannel(4);
                image::save_img(img, dir + "/IR4", false);
                img.clear();

                lines = 0;
                image_vis.clear();
                image_ir1.clear();
                image_ir2.clear();
                image_ir3.clear();
                image_ir4.clear();

                should_save = false;
            }
        }
    }
}