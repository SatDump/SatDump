#include "3mi_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"
#include "common/tracking/interpolator.h"
#include "image/image.h"
#include "image/io.h"
#include <cstdio>

namespace metopsg
{
    namespace threemi
    {
        ThreeMIReader::ThreeMIReader() { img_vec = std::vector<uint8_t>(520 * 520 * 2); }

        ThreeMIReader::~ThreeMIReader() {}

        void ThreeMIReader::work(ccsds::CCSDSPacket &pkt)
        {
            if (pkt.header.apid == 1217)
            {
                if (pkt.payload.size() < 544)
                    return;

                int mkr = pkt.payload[21];

                if (mkr > 9 || mkr < 1)
                    return;

                memcpy(&img_vec[(mkr - 1) * (60636 - 6 - 534 - 4)], &pkt.payload[534 + 2], pkt.payload.size() - 540 - 4);

                if (mkr == 9)
                {
                    image::Image img(16, 520, 520, 1);
                    for (int i = 0; i < 520 * 520; i++)
                        img.set(i, img_vec[i * 2 + 0] << 8 | img_vec[i * 2 + 1]);
                    //  image::normalize(img);

                    image::Image img1(16, 520 / 2, 520, 1);
                    image::Image img2(16, 520 / 2, 520, 1);
                    for (int i = 0; i < 520 * 520 * 0.5; i++)
                    {
                        img1.set(i, img.get(i * 2 + 0));
                        img2.set(i, img.get(i * 2 + 1));
                    }

                    img_n++;

                    image::Image imgf(16, 520, 520, 1);
                    imgf.draw_image(0, img1, 0);
                    img2.mirror(true, false);
                    imgf.draw_image(0, img2, 260);
                    image::save_png(imgf, directory + "/3MI-" + std::to_string(img_n) + ".png");

                    // data_ou.write((char *)img_vec.data() + 4320, img_vec.size());

                    for (auto &v : img_vec)
                        v = 0;
                }
            }
        }

    } // namespace threemi
} // namespace metopsg