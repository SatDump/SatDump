#pragma once

#include "image/image.h"
#include "image/io.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace uvsq
{
    class JPEGImgDecoder
    {
    private:
        std::string directory;
        std::vector<uint8_t> payload;
        int img_n = 0;

    public:
        JPEGImgDecoder(std::string directory) : directory(directory) {}

        void work(uint8_t *cadu)
        {
            int cnt = int(cadu[35] << 8 | cadu[36]);

            if (cnt == 3)
                save();

            cnt -= 3;
            if (cnt >= 0)
            {
                if (payload.size() < (cnt + 1) * 180)
                    payload.resize((cnt + 1) * 180);
                memcpy(&payload[cnt * 180], &cadu[37], 180);
            }
        }

        void save()
        {
            image::Image img;
            image::load_jpeg(img, payload.data(), payload.size());

            //  std::ofstream("/tmp/uv/" + std::to_string(img_n) + ".jpg").write((char *)wip_img.data(), wip_img.size());
            image::save_img(img, directory + "/" + std::to_string(img_n) + ".png");

            img_n++;
            payload.clear();
        }
    };
} // namespace uvsq
