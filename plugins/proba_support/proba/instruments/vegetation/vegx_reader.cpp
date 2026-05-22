#include "vegx_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"

#include "image/image.h"
#include "image/io.h"
#include "logger.h"
#include <array>
#include <cstdint>
#include <fstream>

#include "image/processing.h"

#include "libs/bpe/bpe.h"

// #include "libs/libter/default_values.h"

namespace proba
{
    namespace vegetation
    {
        VegetationX::VegetationX(int seg_size_x, int seg_size_y, int segs_x)
        {
            this->seg_size_x = seg_size_x;
            this->seg_size_y = seg_size_y;
            this->segs_x = segs_x;
        }

        VegetationX::~VegetationX() {}

        image::Image VegetationX::decompressBPE(std::vector<uint8_t> &pkt)
        {
            auto ctx = bpe_context_create();

            // std::ofstream("/tmp/pv_bug.bpe", std::ios::binary).write((char *)pkt.data(), pkt.size());

            void *pixel_ctx = nullptr;
            size_t out_size = 0;
            int ok = bpe_decode(ctx, pkt.data(), pkt.size(), &pixel_ctx, &out_size);

            image::Image img_out;

            if (ok == BPE_OK)
            {
                img_out = image::Image(pixel_ctx, bpe_get_pixel_depth(ctx) > 8 ? 16 : 8, bpe_get_width(ctx), bpe_get_height(ctx), 1);
                img_out = img_out.to16bits();
                free(pixel_ctx);
                // logger->info("BPE Decompression OK!");
                // image::save_img(img_out, "/tmp/img2.png");

                for (size_t v = 0; v < img_out.size(); v++)
                    img_out.set(v, img_out.get(v) << 4);
            }
            else
            {
                logger->info("BPE Decompression Error!");
            }

            bpe_context_destroy(ctx);

            return img_out;
        }

        void VegetationX::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 100)
                return;

            int segment_line_counter = packet.payload[0] << 23 | packet.payload[1] << 15 | packet.payload[2] << 7 | packet.payload[3] >> 1;
            int segment_x_counter = packet.payload[12 - 6] >> 4;

            if (segment_x_counter > (segs_x - 1))
                return;

            if (img_segments.count(segment_line_counter) == 0)
                img_segments.insert({segment_line_counter, std::array<image::Image, 15>()});

            std::vector<uint8_t> vec(&packet.payload[18], &packet.payload[18] + (packet.payload.size() - 18));
            img_segments[segment_line_counter][segment_x_counter] = decompressBPE(vec);
        }

        image::Image VegetationX::getImg()
        {
            // logger->critical("SIZE %d %d", seg_size_x * segs_x, seg_size_y * img_segments.size());

            image::Image img_out(16, seg_size_x * segs_x, seg_size_y * img_segments.size(), 1);

            int i = 0;
            for (auto &map_el : img_segments)
            {
                for (int c = 0; c < segs_x; c++)
                    if (map_el.second[c].size())
                        img_out.draw_image(0, map_el.second[c], c * seg_size_x, i * seg_size_y);
                i++;
            }

            // image::normalize(img_out);

            return img_out;
        }
    } // namespace vegetation
} // namespace proba
