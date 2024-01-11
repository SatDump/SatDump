#pragma once

#include <map>
#include <mutex>
#include <array>
#include "common/ccsds/ccsds.h"

class CCSDSMuxer
{
private:
    const size_t MAX_IN_BUF_CCSDS;
    const size_t MAX_IN_BUF_VCID;
    const int MPDU_TSIZE = 1024;
    const int MPDU_DSIZE = 882;

    struct VCIDStatus
    {
        std::vector<std::array<uint8_t, 1024>> buffer;

        uint32_t vcdu_counter = 0;
        std::array<uint8_t, 1024> wip_frame;
        int first_header = -1;
        int last_position = 0;
    };

    std::mutex vcdu_buf_mtx;
    std::map<int, VCIDStatus> vcdu_status;

    std::mutex ccsds_buf_mtx;
    std::map<int, std::vector<ccsds::CCSDSPacket>> ccsds_buffer;

    std::vector<int> all_vcids;

    uint32_t vcdu_counter_filler = 0;

private:
    void updateVCID(int vcid);
    void make_filler(uint8_t *frm);

public:
    CCSDSMuxer(int max_packets_in_buffer = 500, int max_vcdu_in_buffer = 1000) : MAX_IN_BUF_CCSDS(max_packets_in_buffer), MAX_IN_BUF_VCID(max_vcdu_in_buffer) {}
    bool pushPacket(int vcid, ccsds::CCSDSPacket &pkt);
    void work(uint8_t *vcdus, int cnt); // Generate the requested amount of CADUs. If no data is present, fill will be added.
};