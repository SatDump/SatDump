#pragma once

#include "image/image.h"
#include "image/io.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace uvsq
{
    class RawImgDecoder
    {
    private:
        std::string directory;
        std::vector<uint8_t> payload;
        int img_n = 0;

    public:
        RawImgDecoder(std::string directory) : directory(directory) {}

        void work(uint8_t *cadu)
        {
            int cnt = int(cadu[35] << 8 | cadu[36]);

            if (cnt == 1)
                save();

            if (cnt >= 0)
            {
                if (payload.size() < (cnt + 1) * 188)
                    payload.resize((cnt + 1) * 188);
                memcpy(&payload[cnt * 188], &cadu[37], 188);
            }
        }

        void save()
        {
            int width = ((1024 * 16 - 16) / 16);
            payload.resize(width * 1083 + 14);

            image::Image img(payload.data() + 14, 16, width, 1083, 1);

            //  std::ofstream("/tmp/uv/" + std::to_string(img_n) + ".jpg").write((char *)wip_img.data(), wip_img.size());
            image::save_img(img, directory + "/" + std::to_string(img_n) + ".png");

            img_n++;
            payload.clear();
        }
    };
} // namespace uvsq
