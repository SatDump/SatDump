#include "csr_lrpt.h"
#include "common/ccsds/ccsds.h"
#include "image/image.h"
#include "image/io.h"
#include "utils/time.h"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <unistd.h>

namespace satdump
{
    namespace ominous
    {
        CSRLRPTSimulator::CSRLRPTSimulator(WorkerCore core) : InstrumentWorkerBase(core)
        {
            core.pkt_bus->register_handler<PushSpacePacketEvent>(
                [this](PushSpacePacketEvent e)
                {
                    if (e.vcid == 5 && e.pkt.header.apid == 1)
                    {
                        pkt_vec_mtx.lock();
                        pkt_vec.push_back(e.pkt);
                        pkt_vec_mtx.unlock();
                    }
                });

            for (int i = 0; i < 10; i++)
            {
                img[i] = image::Image(16, 2048, 64, 1);
            }

            start_th();
        }

        void CSRLRPTSimulator::loop()
        {
            ccsds::CCSDSPacket csrpkt;

            pkt_vec_mtx.lock();
            if (pkt_vec.size())
            {
                csrpkt = pkt_vec[0];
                pkt_vec.erase(pkt_vec.begin(), pkt_vec.begin() + 1);
                pkt_vec_mtx.unlock();
            }
            else
            {
                pkt_vec_mtx.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                return;
            }

            for (int c = 0; c < 10; c++)
            {
                for (int i = 0; i < 2048; i++)
                {
                    img[c].set(line * 2048 + i, csrpkt.payload[8 + (i * 10 + c) * 2 + 0] << 8 | csrpkt.payload[8 + (i * 10 + c) * 2 + 1]);
                }
            }

            if (line == 0)
            {
                for (int i = 0; i < 8; i++)
                    ((uint8_t *)&line0_time)[7 - i] = csrpkt.payload[0 * 8 + i];
            }

            line++;

            if (line >= 64)
            {
                line = 0;

                std::vector<ccsds::CCSDSPacket> pkts;

                std::vector<float> ratios = {
                    2.0, //
                    2.0, //
                    1.0, //
                    1.0, //
                    1.0, //
                    1.0, //
                    1.0, //
                    1.0, //
                    1.0, //
                    1.0, //
                };

                std::vector<float> resize = {
                    1, //
                    1, //
                    2, //
                    2, //
                    2, //
                    2, //
                    2, //
                    2, //
                    2, //
                    2, //
                };

                int fsize = 0;

                for (int s = 0; s < 16; s++)
                {
                    for (int c = 0; c < 10; c++)
                    {
                        ccsds::CCSDSPacket pkt;
                        pkt.header.version = 0;
                        pkt.header.type = 0;
                        pkt.header.secondary_header_flag = true;
                        pkt.header.apid = 101 + c; // IDK
                        pkt.header.sequence_flag = 0b11;
                        pkt.header.packet_sequence_count = pkt_index_seq;

                        int downscale = resize[c];

                        pkt.payload.resize(8 + 2);
                        for (int i = 0; i < 8; i++)
                            pkt.payload[0 * 8 + i] = ((uint8_t *)&line0_time)[7 - i];

                        pkt.payload[8] = downscale;
                        pkt.payload[9] = s;

                        auto img2 = img[c].to16bits();

                        img2.crop((s + 0) * 128, (s + 1) * 128);

                        img2.resize_bilinear(img2.width() / downscale, img2.height() / downscale);

                        // image::save_png(img2, "/tmp/csr_test_segs/ch" + std::to_string(c + 1) + "/" + std::to_string((time_t)line0_time) + ".png");

                        std::ofstream("/tmp/lrpt_bpe.in").write((char *)img2.raw_data(), img2.width() * img2.height() * sizeof(uint16_t));
                        std::string command = "/home/alan/Downloads/BPE.MOD/bin/bpe -e /tmp/lrpt_bpe.in -o /tmp/lrpt_bpe.out -r " + std::to_string(ratios[c]) + " -b 16 -w " +
                                              std::to_string(img2.width()) + " -h " + std::to_string(img2.height());
                        //  logger->info(command);
                        system(command.c_str());

                        std::vector<uint8_t> v;
                        {
                            std::ifstream input_file("/tmp/lrpt_bpe.out", std::ios::binary);
                            input_file.seekg(0, std::ios::end);
                            const size_t fsize = input_file.tellg();
                            v.resize(fsize);
                            input_file.seekg(0, std::ios::beg);
                            input_file.read((char *)&v[0], fsize);
                            input_file.close();
                        }
                        pkt.payload.insert(pkt.payload.end(), v.begin(), v.end());

                        fsize += pkt.payload.size() + 6;
                        pkts.push_back(pkt);
                    }

                    pkt_index_seq++;
                }

                logger->info("Bandwidth used : %f kbps", (double(fsize * 8) / (64.0 * 0.2)) / 1e3);

                for (auto &p : pkts)
                {
                    core.pushPkt(5, p);
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
            }
        }
    } // namespace ominous
} // namespace satdump