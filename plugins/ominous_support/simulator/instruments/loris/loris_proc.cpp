#include "loris_proc.h"
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
        LORISProcessor::LORISProcessor(WorkerCore core) : InstrumentWorkerBase(core)
        {
            core.pkt_bus->register_handler<PushSpacePacketEvent>(
                [this](PushSpacePacketEvent e)
                {
                    if (e.vcid == 6 && e.pkt.header.apid == 1)
                    {
                        pkt_vec_mtx.lock();
                        pkt_vec.push_back(e.pkt);
                        pkt_vec_mtx.unlock();
                    }
                });

            start_th();
        }

        void LORISProcessor::loop()
        {
            ccsds::CCSDSPacket lorispkt;

            pkt_vec_mtx.lock();
            if (pkt_vec.size())
            {
                lorispkt = pkt_vec[0];
                pkt_vec.erase(pkt_vec.begin(), pkt_vec.begin() + 1);
                pkt_vec_mtx.unlock();
            }
            else
            {
                pkt_vec_mtx.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                return;
            }

            ccsds::CCSDSPacket pkt;
            pkt.header.version = 0;
            pkt.header.type = 0;
            pkt.header.secondary_header_flag = true;
            pkt.header.apid = 2; // IDK
            pkt.header.sequence_flag = 0b11;
            pkt.header.packet_sequence_count = pkt_index_seq;

            pkt.payload.resize(8);
            for (int i = 0; i < 8; i++)
                pkt.payload[i] = lorispkt.payload[i];

            image::Image img2(16, 160, 120, 1);
            memcpy(img2.raw_data(), &lorispkt.payload[8], 160 * 120 * 2);

            // image::save_png(img2, "/tmp/csr_test_segs/ch" + std::to_string(c + 1) + "/" + std::to_string((time_t)line0_time) + ".png");

            std::ofstream("/tmp/lrpt_bpe.in").write((char *)img2.raw_data(), img2.width() * img2.height() * sizeof(uint16_t));
            std::string command = "/home/alan/Downloads/BPE.MOD/bin/bpe -e /tmp/lrpt_bpe.in -o /tmp/lrpt_bpe.out -b 16 -w " + std::to_string(img2.width()) + " -h " + std::to_string(img2.height());
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

            logger->info("LORIS Compressed size %d/%d (%f)", v.size(), 160 * 120 * 2, double(v.size()) / double(160 * 120 * 2));

            pkt_index_seq++;

            core.pushPkt(6, pkt);
        }
    } // namespace ominous
} // namespace satdump