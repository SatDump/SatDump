#pragma once

#include "image/image.h"
#include "image/io.h"
#include "logger.h"
#include "utils/stats.h"
#include "utils/time.h"
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace uvsq
{
    class ImgPayloadDecoder
    {
    private:
        const int d_mode;

        std::string directory;

        struct ImgDat
        {
            std::vector<bool> c;
            std::vector<uint8_t> p;
            std::vector<double> t;
        };
        std::map<int, ImgDat> payloads;

    public:
        ImgPayloadDecoder(int mode, std::string directory) : d_mode(mode), directory(directory) {}

        void work(uint8_t *cadu)
        {
            uint32_t timestamp = cadu[23] << 24 | cadu[24] << 16 | cadu[25] << 8 | cadu[26];
            int id = int(cadu[33] << 8 | cadu[34]);
            int cnt = int(cadu[35] << 8 | cadu[36]);

            if (payloads.count(id) == 0)
                payloads.emplace(id, ImgDat());

            auto &payload = payloads[id];

            int chunksize = 0;
            if (d_mode == 0) // JPEG Mode
                chunksize = 180;
            else if (d_mode == 1) // RAW Mode
                chunksize = 188;

            if (d_mode == 0) // JPEG starts at an offset
                cnt -= 3;

            if (cnt >= 0)
            {
                if (payload.p.size() < (cnt + 1) * chunksize)
                {
                    payload.p.resize((cnt + 1) * chunksize);
                    payload.c.resize(cnt + 1, false);
                }
                memcpy(&payload.p[cnt * chunksize], &cadu[37], chunksize);
                payload.c[cnt] = true;
                payload.t.push_back(timestamp);
            }
        }

        void save()
        {
            for (auto &p : payloads)
            {
                image::Image img;

                if (d_mode == 0)
                {
                    image::load_jpeg(img, p.second.p.data(), p.second.p.size());
                }
                else if (d_mode == 1)
                {
                    int width = ((1024 * 16 - 16) / 16);
                    p.second.p.resize(width * 1083 + 14);
                    img = image::Image(p.second.p.data() + 14, 16, width, 1083, 1);
                }

                double timestamp = satdump::get_median(p.second.t);

                int valid = 0;
                for (auto v : p.second.c)
                    valid += int(v);
                logger->info("Image ID %d, Got %d/%d Chunks, Time %s", p.first, valid, p.second.c.size(), satdump::timestamp_to_string(timestamp).c_str());

                // std::ofstream(directory + "/" + std::to_string(p.first) + ".jpg").write((char *)p.second.p.data(), p.second.p.size());

                image::save_img(img, directory + "/" + std::to_string(p.first) + ".png");
            }
        }
    };
} // namespace uvsq
