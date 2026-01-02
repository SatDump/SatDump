#include "csr.h"
#include "common/ccsds/ccsds.h"
#include "image/io.h"
#include "utils/time.h"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <thread>

namespace satdump
{
    namespace ominous
    {
        CSRSimulator::CSRSimulator(WorkerCore core)
            : InstrumentWorkerBase(core), //
              simulator_csr(core.tracker, 110, 2048, "/home/alan/Downloads/CloudyBlueMarble.png")
        {
            // /home/alan/Documents/SatDump/build/fengyun3d_andrew_tune/MERSI-2/
            image::load_img(test_channels[0], "/home/alan/Downloads/202509081422_FengYun-3D/MERSI-2/MERSI2-3.pbm");
            image::load_img(test_channels[1], "/home/alan/Downloads/202509081422_FengYun-3D/MERSI-2/MERSI2-4.pbm");
            image::load_img(test_channels[2], "/home/alan/Downloads/202509081422_FengYun-3D/MERSI-2/MERSI2-8.pbm");
            image::load_img(test_channels[3], "/home/alan/Downloads/202509081422_FengYun-3D/MERSI-2/MERSI2-13.pbm");
            image::load_img(test_channels[4], "/home/alan/Downloads/202509081422_FengYun-3D/MERSI-2/MERSI2-1.pbm");
            image::load_img(test_channels[5], "/home/alan/Downloads/202509081422_FengYun-3D/MERSI-2/MERSI2-2.pbm");
            image::load_img(test_channels[6], "/home/alan/Downloads/202509081422_FengYun-3D/MERSI-2/MERSI2-25.pbm");
            image::load_img(test_channels[7], "/home/alan/Downloads/202509081422_FengYun-3D/MERSI-2/MERSI2-12.pbm");
            image::load_img(test_channels[8], "/home/alan/Downloads/202509081422_FengYun-3D/MERSI-2/MERSI2-10.pbm");
            image::load_img(test_channels[9], "/home/alan/Downloads/202509081422_FengYun-3D/MERSI-2/MERSI2-5.pbm");

            for (int i = 0; i < 10; i++)
            {
                double ratio = 2048.0 / test_channels[i].width();
                test_channels[i].resize_bilinear(2048, test_channels[i].height() * ratio);
                test_channels[i] = test_channels[i].to16bits();
            }

            start_th();
            start_time = getTime();
        }

        void CSRSimulator::loop()
        {
            double time = getTime();
            auto pos = core.tracker->get_sat_position_at_raw(time);

            ccsds::CCSDSPacket pkt;
            pkt.header.version = 0;
            pkt.header.type = 0;
            pkt.header.secondary_header_flag = false;
            pkt.header.apid = 1;
            pkt.header.sequence_flag = 0b11;
            pkt.header.packet_sequence_count = pkt_index_seq;
            pkt_index_seq++;

            pkt.payload.resize(8 + 2048 * 2 * 10 + 2);

            for (int i = 0; i < 8; i++)
                pkt.payload[0 * 8 + i] = ((uint8_t *)&time)[7 - i];

            uint16_t img_channels[10][2048];

            if (false)
            {
                uint16_t img_r[2048];
                uint16_t img_g[2048];
                uint16_t img_b[2048];
                simulator_csr.get_scanline(time,              //
                                           (uint16_t *)img_r, //
                                           (uint16_t *)img_g, //
                                           (uint16_t *)img_b);

                memcpy(img_channels[0], img_r, 2 * 2048);
                memcpy(img_channels[1], img_g, 2 * 2048);
                memcpy(img_channels[2], img_b, 2 * 2048);
                memcpy(img_channels[3], img_r, 2 * 2048);
                memcpy(img_channels[4], img_g, 2 * 2048);
                memcpy(img_channels[5], img_b, 2 * 2048);
                memcpy(img_channels[6], img_r, 2 * 2048);
                memcpy(img_channels[7], img_g, 2 * 2048);
                memcpy(img_channels[8], img_b, 2 * 2048);
                memcpy(img_channels[9], img_r, 2 * 2048);
            }
            else
            {
                for (int c = 0; c < 10; c++)
                {
                    for (int i = 0; i < 2048; i++)
                    {
                        img_channels[c][i] = test_channels[c].get(test_channels_line * 2048 + i);
                    }
                }

                test_channels_line++;
                if (test_channels_line == test_channels[0].height())
                    test_channels_line = 0;
            }

            for (int c = 0; c < 10; c++)
            {
                for (int i = 0; i < 2048; i++)
                {
                    pkt.payload[8 + (i * 10 + c) * 2 + 0] = img_channels[c][i] >> 8;
                    pkt.payload[8 + (i * 10 + c) * 2 + 1] = img_channels[c][i] & 0xFF;
                }
            }

            core.pushPkt(5, pkt);

            time += 0.2;
            std::this_thread::sleep_until(std::chrono::system_clock::from_time_t(floor(time)) + std::chrono::milliseconds(int(fmod(time, 1) * 1e3)));
        }
    } // namespace ominous
} // namespace satdump