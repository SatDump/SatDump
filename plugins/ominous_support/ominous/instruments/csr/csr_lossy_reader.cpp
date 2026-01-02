#include "csr_lossy_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"
#include "image/image.h"
#include "image/io.h"
#include "logger.h"

#include "common/utils.h"
#include <cstdint>
#include <libheif/heif_context.h>
#include <libheif/heif_decoding.h>
#include <libheif/heif_image.h>
#include <libheif/heif_image_handle.h>
#include <map>

namespace ominous
{
    namespace csr
    {
        CSRLossyReader::CSRLossyReader() {}

        CSRLossyReader::~CSRLossyReader() {}

        void CSRLossyReader::work(ccsds::CCSDSPacket &pkt)
        {
            if (pkt.payload.size() < 10)
                return;

            if (pkt.header.apid >= 101 && pkt.header.apid <= 110)
            {
                int channel = pkt.header.apid - 101;

                CSRLossySegment seg;

                for (int i = 0; i < 8; i++)
                    ((uint8_t *)&seg.timestamp)[7 - i] = pkt.payload[0 * 8 + i];

                int downscale = pkt.payload[8];
                seg.pos = pkt.payload[9];

                {
                    std::ofstream("/tmp/bpe2.in").write((char *)pkt.payload.data() + 10, pkt.payload.size() - 10);
                    std::string command_decomp = "/home/alan/Downloads/BPE.MOD/bin/bpe -d /tmp/bpe2.in -o /tmp/bpe2.out -b 16 2>&1 > /dev/null";
                    // logger->info(command_decomp);
                    system(command_decomp.c_str());

                    // system("du -b /tmp/bpe.out");

                    image::Image img_e(16, 128 / downscale, 64 / downscale, 1);
                    img_e.fill(0);
                    std::ifstream("/tmp/bpe2.out").read((char *)img_e.raw_data(), img_e.width() * img_e.height() * sizeof(uint16_t));

                    // image::save_png(img_e, "/tmp/csr_test_segs/ch" + std::to_string(channel + 1) + "/dec_" + std::to_string((time_t)seg.timestamp) + ".png");

                    img_e.resize_bilinear(img_e.width() * downscale, img_e.height() * downscale);

                    seg.img = img_e;
                }

                channels[channel].push_back(seg);

                if (channel == 0)
                    segments++;
            }
        }

        image::Image CSRLossyReader::getChannel(int channel)
        {

            std::vector<double> timestamps;
            for (auto &s : channels[channel])
            {
                bool contains = false;
                for (auto &t : timestamps)
                    if (t == s.timestamp)
                        contains = true;

                if (!contains)
                    timestamps.push_back(s.timestamp);
            }

            image::Image img(16, 2048, timestamps.size() * 64, 1);

            for (int f = 0; f < timestamps.size(); f++)
            {
                for (auto &s : channels[channel])
                {
                    if (timestamps[f] == s.timestamp)
                    {
                        for (int l = 0; l < 64; l++)
                        {
                            for (int i = 0; i < 128; i++)
                            {
                                if (s.pos < 16)
                                    if (s.img.width() == 128 && s.img.height() == 64)
                                        img.set(0, i + s.pos * 128, f * 64 + l, s.img.get(0, i, l));
                            }
                        }
                    }
                }
            }

            return img;
        }
    } // namespace csr
} // namespace ominous