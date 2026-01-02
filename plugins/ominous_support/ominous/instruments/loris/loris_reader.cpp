#include "loris_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"
#include "image/image.h"
#include "image/io.h"
#include "logger.h"

#include "common/utils.h"
#include <cstring>
#include <string>

#include "image/processing.h"

namespace ominous
{
    namespace loris
    {
        LORISReader::LORISReader() { images = 0; }

        LORISReader::~LORISReader() {}

        void LORISReader::work(ccsds::CCSDSPacket &pkt)
        {
            if (pkt.header.apid == 1)
            {
                if (pkt.payload.size() < 38410)
                    return;

                image::Image img(16, 160, 120, 1);
                memcpy(img.raw_data(), &pkt.payload[8], 160 * 120 * 2);

                image::normalize(img);
                image::save_png(img, directory + "/" + std::to_string(images + 1) + ".png");

                images++;
            }
            else if (pkt.header.apid == 2)
            {
                if (pkt.payload.size() < 9)
                    return;

                {
                    std::ofstream("/tmp/bpe2.in").write((char *)pkt.payload.data() + 8, pkt.payload.size() - 8);
                    std::string command_decomp = "/home/alan/Downloads/BPE.MOD/bin/bpe -d /tmp/bpe2.in -o /tmp/bpe2.out -b 16";
                    // logger->info(command_decomp);
                    system(command_decomp.c_str());

                    // system("du -b /tmp/bpe.out");

                    image::Image img_e(16, 160, 120, 1);
                    img_e.fill(0);
                    std::ifstream("/tmp/bpe2.out").read((char *)img_e.raw_data(), img_e.width() * img_e.height() * sizeof(uint16_t));

                    // image::save_png(img_e, "/tmp/csr_test_segs/ch" + std::to_string(channel + 1) + "/dec_" + std::to_string((time_t)seg.timestamp) + ".png");

                    image::normalize(img_e);
                    image::save_png(img_e, directory + "/" + std::to_string(images + 1) + ".png");

                    images++;
                }
            }
        }
    } // namespace loris
} // namespace ominous