#include "ccsds_muxer.h"
#include <algorithm>
#include <cstring>

// #include "logger.h"

bool CCSDSMuxer::pushPacket(int vcid, ccsds::CCSDSPacket &pkt)
{
    if (vcdu_status.count(vcid) == 0)
    {
        vcdu_buf_mtx.lock();
        if (vcdu_status.count(vcid) == 0)
            vcdu_status.emplace(vcid, VCIDStatus(MPDU_TSIZE));
        all_vcids.push_back(vcid);
        std::sort(all_vcids.begin(), all_vcids.end());
        vcdu_buf_mtx.unlock();
        //  logger->critical("VCID UPDATE {:d}", vcid);
    }

    bool v = !vcid_queues[vcid]->try_push(pkt);

    return v;
}

void CCSDSMuxer::work(uint8_t *vcdus, int cnt)
{
    for (int i = 0; i < cnt; i++)
    {
        bool has_one = false;
        vcdu_buf_mtx.lock();
        for (int vcid : all_vcids)
        {
            updateVCID(vcid);

            // logger->trace("VCID UPDATE {:d}", vcid);

            if (vcdu_status[vcid].buffer.size() > 0)
            {
                // logger->critical("GOT VCID !!!");
                auto &vcc = vcdu_status[vcid].buffer[0];
                vcdus[i * MPDU_TSIZE + 0] = 0x1a;
                vcdus[i * MPDU_TSIZE + 1] = 0xcf;
                vcdus[i * MPDU_TSIZE + 2] = 0xfc;
                vcdus[i * MPDU_TSIZE + 3] = 0x1d;
                memcpy(&vcdus[i * MPDU_TSIZE + 4], vcc.data(), MPDU_TSIZE - 4);
                vcdu_status[vcid].buffer.erase(vcdu_status[vcid].buffer.begin(), vcdu_status[vcid].buffer.begin() + 1);
                vcdu_status[vcid].buffer.shrink_to_fit();
                has_one = true;
            }

            if (has_one)
                break;
        }
        vcdu_buf_mtx.unlock();

        if (!has_one)
        {
            // If we get there, filler.
            make_filler(&vcdus[i * MPDU_TSIZE + 4]);
            vcdus[i * MPDU_TSIZE + 0] = 0x1a;
            vcdus[i * MPDU_TSIZE + 1] = 0xcf;
            vcdus[i * MPDU_TSIZE + 2] = 0xfc;
            vcdus[i * MPDU_TSIZE + 3] = 0x1d;
            // printf("FILLER\n");
        }
    }
}

void CCSDSMuxer::updateVCID(int vcid)
{
    // ccsds_buf_mtx[vcid].lock();
    // int pktn = 0;
    bool has_pkt = true;
    while (has_pkt /*&& !will_have_more_soon*/ && vcdu_status[vcid].buffer.size() <= MAX_IN_BUF_VCID)
    {
        // auto &pkt = ccsds_buffer[vcid][pktn];

        //  if (vcid != 0)
        //      logger->trace("IDK");

        ccsds::CCSDSPacket pkt;
        has_pkt = vcid_queues[vcid]->try_pop(pkt);
        // if (vcid != 0 && !has_pkt)
        //     logger->trace(has_pkt);
        if (!has_pkt)
            continue;

        // if (vcid != 0)
        //     logger->trace("IDK2 {:d} {:d}", vcid, pkt.header.apid);

        // if (vcid == 8)
        //     logger->warn("{:d},{:d} - {:d} BUFSIZE {:d} - {:d}", vcid, pkt.header.apid, vcid_queues[vcid]->size(), vcdu_status[vcid].buffer.size(), pkt.header.packet_sequence_count);

        // vcdu_buf_mtx.lock();

        auto &vcdu_sts = vcdu_status[vcid];

        // Convert that PKT into a simple vector, simpler
        pkt.encodeHDR();
        auto &fpkt = pkt.payload;
        fpkt.insert(fpkt.begin(), &pkt.header.raw[0], &pkt.header.raw[6]);

        bool wrote_pkt_hdr = false;

        // int fpkt_pos = 0;
        while (fpkt.size() > 0)
        {
            uint8_t *phdr = &vcdu_sts.wip_frame[0];
            // uint8_t *izon = &vcdu_sts.wip_frame[6];
            uint8_t *idat = &vcdu_sts.wip_frame[6];

            if (vcdu_sts.first_header == -1 // We do NOT have a header in this VCDU yet
                && !wrote_pkt_hdr)          // Neither did we write our space packet header
            {
                int to_write = std::min<int>(fpkt.size(), MPDU_DSIZE - vcdu_sts.last_position);
                memcpy(&idat[2 + vcdu_sts.last_position], &fpkt[0], to_write);
                vcdu_sts.first_header = vcdu_sts.last_position;
                vcdu_sts.last_position += to_write;
                fpkt.erase(fpkt.begin(), fpkt.begin() + to_write);
                wrote_pkt_hdr = true;

                // logger->info(to_write);
            }
            else if (vcdu_sts.first_header >= 0 // We do have a header in this VCDU
                     && !wrote_pkt_hdr)         // but didn't write our space packet header yet
            {
                int to_write = std::min<int>(fpkt.size(), MPDU_DSIZE - vcdu_sts.last_position);
                memcpy(&idat[2 + vcdu_sts.last_position], &fpkt[0], to_write);
                vcdu_sts.last_position += to_write;
                fpkt.erase(fpkt.begin(), fpkt.begin() + to_write);
                wrote_pkt_hdr = true;

                // logger->info(to_write);
            }
            else if (vcdu_sts.first_header == -1 // We don't have a header in this VCDU
                     && wrote_pkt_hdr)           // but did write our space packet header already
            {
                int to_write = std::min<int>(fpkt.size(), MPDU_DSIZE - vcdu_sts.last_position);
                memcpy(&idat[2 + vcdu_sts.last_position], &fpkt[0], to_write);
                vcdu_sts.last_position += to_write;
                fpkt.erase(fpkt.begin(), fpkt.begin() + to_write);

                // logger->info(to_write);
            }

            // logger->critical("RUN {:d} {:d}", vcdu_sts.first_header, wrote_pkt_hdr);

#if 0
            if (is_at_last_of_buf)
            {
                bool had_more = ccsds_buffer[vcid].size() > 0;
                //  ccsds_buf_mtx[vcid].unlock();
                //  ccsds_buf_mtx[vcid].lock();
                bool has_more = ccsds_buffer[vcid].size() > 0;

                //  ccsds_buffer[vcid].erase(ccsds_buffer[vcid].begin(), ccsds_buffer[vcid].begin() + pktn);
                //  pktn = 0;

                will_have_more_soon = has_more; // !had_more && has_more; // Ensure we can stop but carry on later without losing BW
            }
#endif

            // if (vcid == 8)
            //     logger->critical("FPT {:d} {:d} {:d}", 0, will_have_more_soon, vcid_queues[vcid]->size());

            if (vcdu_sts.last_position >= MPDU_DSIZE || (vcid_queues[vcid]->size() == 0 && fpkt.size() == 0))
            {
                // VCDU Primary Header
                uint8_t version_no = 1;
                uint16_t scid = 69;
                uint8_t vcid_type = vcid;
                uint32_t vcdu_counter = vcdu_sts.vcdu_counter++;
                bool replay_flag = false;

                phdr[0] = (version_no & 0b11) << 6 | ((scid >> 2) & 0b111111);
                phdr[1] = (scid & 0b11) << 6 | (vcid_type & 0b111111);
                phdr[2] = (vcdu_counter >> 16) & 0xFF;
                phdr[3] = (vcdu_counter >> 8) & 0xFF;
                phdr[4] = vcdu_counter & 0xFF;
                phdr[5] = (replay_flag & 0b1) << 7 | 0b0000000;

                // Insert Zone
                // izon[0] = 0x00;
                // izon[1] = 0x00;

                // M-PDU
                int first_header_pointer = vcdu_sts.first_header == -1 ? 2047 : vcdu_sts.first_header;

                idat[0] = (first_header_pointer >> 8) & 0b111;
                idat[1] = first_header_pointer & 0xFF;

                // logger->info("{:d} {:d}", vcdu_sts.last_position, int(!will_have_more_soon && is_at_last_of_buf));

                // if (vcid == 8)
                //      logger->critical("OUTPUT VCDU {:d} - {:d} {:d}   {:d}", vcdu_sts.vcdu_counter, vcdu_sts.last_position, first_header_pointer, fpkt.size());

                vcdu_sts.buffer.push_back(vcdu_sts.wip_frame);
                vcdu_sts.last_position = 0;
                vcdu_sts.first_header = -1;
                memset(vcdu_sts.wip_frame.data(), 0, MPDU_TSIZE);
            }
        }

        // vcdu_buf_mtx.unlock();

        // ccsds_buffer[vcid].erase(ccsds_buffer[vcid].begin(), ccsds_buffer[vcid].begin() + 1);
        //   pktn++;
    }
    // ccsds_buffer[vcid].erase(ccsds_buffer[vcid].begin(), ccsds_buffer[vcid].begin() + pktn);
    // ccsds_buf_mtx[vcid].unlock();
}

void CCSDSMuxer::make_filler(uint8_t *frm)
{
    uint8_t *phdr = &frm[0];
    // uint8_t *izon = &frm[6];
    uint8_t *idat = &frm[6];

    // VCDU Primary Header
    uint8_t version_no = 1;
    uint16_t scid = 69;
    uint8_t vcid_type = 63;
    uint32_t vcdu_counter = vcdu_counter_filler++;
    bool replay_flag = false;

    phdr[0] = (version_no & 0b11) << 6 | ((scid >> 2) & 0b111111);
    phdr[1] = (scid & 0b11) << 6 | (vcid_type & 0b111111);
    phdr[2] = (vcdu_counter >> 16) & 0xFF;
    phdr[3] = (vcdu_counter >> 8) & 0xFF;
    phdr[4] = vcdu_counter & 0xFF;
    phdr[5] = (replay_flag & 0b1) << 7 | 0b0000000;

    // Insert Zone
    // izon[0] = 0x00;
    // izon[1] = 0x00;

    // M-PDU
    int first_header_pointer = 2047;

    idat[0] = (first_header_pointer >> 8) & 0b111;
    idat[1] = first_header_pointer & 0xFF;

    memset(&idat[2], 0x00, MPDU_DSIZE);
}