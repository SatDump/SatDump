#include "ccsds_muxer.h"
#include <cstring>
#include <string>

bool CCSDSMuxer::pushPacket(int vcid, ccsds::CCSDSPacket &pkt)
{
    ccsds_buf_mtx.lock();
    if (ccsds_buffer.count(vcid) == 0)
    {
        ccsds_buffer.emplace(vcid, std::vector<ccsds::CCSDSPacket>());
        all_vcids.push_back(vcid);
    }

    if (ccsds_buffer[vcid].size() >= MAX_IN_BUF_CCSDS)
    {
        ccsds_buf_mtx.unlock();
        return true; // Overflow
    }

    ccsds_buffer[vcid].push_back(pkt);

    ccsds_buf_mtx.unlock();
    return false;
}

void CCSDSMuxer::work(uint8_t *vcdus, int cnt)
{
    for (int i = 0; i < cnt; i++)
    {
        bool has_one = false;
        for (int vcid : all_vcids)
        {
            updateVCID(vcid);

            // logger->info("VCID UPDATE {:d}", vcid);

            vcdu_buf_mtx.lock();
            if (vcdu_status[vcid].buffer.size() > 0)
            {
                // logger->critical("GOT VCID !!!");
                auto vcc = vcdu_status[vcid].buffer[0];
                vcdus[i * 1024 + 0] = 0x1a;
                vcdus[i * 1024 + 1] = 0xcf;
                vcdus[i * 1024 + 2] = 0xfc;
                vcdus[i * 1024 + 3] = 0x1d;
                memcpy(&vcdus[i * 1024 + 4], vcc.data(), 1020);
                vcdu_status[vcid].buffer.erase(vcdu_status[vcid].buffer.begin(), vcdu_status[vcid].buffer.begin() + 1);
                has_one = true;
            }
            vcdu_buf_mtx.unlock();

            if (has_one)
                break;
        }

        if (!has_one)
        {
            // If we get there, filler.
            vcdus[i * 1024 + 0] = 0x1a;
            vcdus[i * 1024 + 1] = 0xcf;
            vcdus[i * 1024 + 2] = 0xfc;
            vcdus[i * 1024 + 3] = 0x1d;
            make_filler(&vcdus[i * 1024 + 4]);
        }
    }
}

void CCSDSMuxer::updateVCID(int vcid)
{
    bool will_have_more_soon = false;
    ccsds_buf_mtx.lock();
    while (ccsds_buffer[vcid].size() > 0 && !will_have_more_soon && vcdu_status[vcid].buffer.size() <= MAX_IN_BUF_VCID)
    {
        auto &pkt = ccsds_buffer[vcid][0];

        vcdu_buf_mtx.lock();

        if (vcdu_status.count(vcid) == 0)
            vcdu_status.emplace(vcid, VCIDStatus());

        auto &vcdu_sts = vcdu_status[vcid];

        // Convert that PKT into a simple vector, simpler
        pkt.encodeHDR();
        auto &fpkt = pkt.payload;
        fpkt.insert(fpkt.begin(), &pkt.header.raw[0], &pkt.header.raw[6]);

        bool wrote_pkt_hdr = false;

        while (fpkt.size() > 0)
        {
            uint8_t *phdr = &vcdu_sts.wip_frame[0];
            uint8_t *izon = &vcdu_sts.wip_frame[6];
            uint8_t *idat = &vcdu_sts.wip_frame[8];

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

            bool is_at_last_of_buf = ccsds_buffer[vcid].size() == 1;

            if (is_at_last_of_buf)
            {
                bool had_more = ccsds_buffer[vcid].size() > 0;
                ccsds_buf_mtx.unlock();
                bool has_more = ccsds_buffer[vcid].size() > 0;
                ccsds_buf_mtx.lock();

                will_have_more_soon = !had_more && has_more; // Ensure we can stop but carry on later without losing BW
            }

            // logger->critical("FPT {:d}", fpkt.size());

            if (vcdu_sts.last_position >= MPDU_DSIZE || (!will_have_more_soon && is_at_last_of_buf && fpkt.size() == 0))
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
                izon[0] = 0x00;
                izon[1] = 0x00;

                // M-PDU
                int first_header_pointer = vcdu_sts.first_header == -1 ? 2047 : vcdu_sts.first_header;

                idat[0] = (first_header_pointer >> 8) & 0b111;
                idat[1] = first_header_pointer & 0xFF;

                // logger->info("{:d} {:d}", vcdu_sts.last_position, int(!will_have_more_soon && is_at_last_of_buf));

                vcdu_sts.buffer.push_back(vcdu_sts.wip_frame);
                vcdu_sts.last_position = 0;
                vcdu_sts.first_header = -1;
                vcdu_sts.wip_frame.fill(0);
            }
        }

        vcdu_buf_mtx.unlock();

        ccsds_buffer[vcid].erase(ccsds_buffer[vcid].begin(), ccsds_buffer[vcid].begin() + 1);
    }
    ccsds_buf_mtx.unlock();
}

void CCSDSMuxer::make_filler(uint8_t *frm)
{
    uint8_t *phdr = &frm[0];
    uint8_t *izon = &frm[6];
    uint8_t *idat = &frm[8];

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
    izon[0] = 0x00;
    izon[1] = 0x00;

    // M-PDU
    int first_header_pointer = 2047;

    idat[0] = (first_header_pointer >> 8) & 0b111;
    idat[1] = first_header_pointer & 0xFF;

    memset(&idat[2], 0x00, 882);

    std::string callsign = "ADDYOURSHERE";
    memcpy(&idat[2], callsign.c_str(), callsign.size());
}