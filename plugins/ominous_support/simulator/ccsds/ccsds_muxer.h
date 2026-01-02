#pragma once

#include "common/ccsds/ccsds.h"
#include "queue.h"
#include <array>
#include <map>
#include <mutex>

class CCSDSMuxer
{
private:
    const size_t MAX_IN_BUF_CCSDS;
    const size_t MAX_IN_BUF_VCID;
    const int MPDU_TSIZE;
    const int MPDU_DSIZE;

    struct VCIDStatus
    {
        std::vector<std::vector<uint8_t>> buffer;

        uint32_t vcdu_counter = 0;
        std::vector<uint8_t> wip_frame;
        int first_header = -1;
        int last_position = 0;

        VCIDStatus() {}
        VCIDStatus(int sz) { wip_frame.resize(sz); }
    };

    std::mutex vcdu_buf_mtx;
    std::map<int, VCIDStatus> vcdu_status;

    rigtorp::MPMCQueue<ccsds::CCSDSPacket> *vcid_queues[64];

    std::vector<int> all_vcids;

    uint32_t vcdu_counter_filler = 0;

private:
    void updateVCID(int vcid);
    void make_filler(uint8_t *frm);

public:
    CCSDSMuxer(int max_packets_in_buffer = 500, int max_vcdu_in_buffer = 1000, int mpdu_tsize = 1024, int mpdu_dsize = 884)
        : MAX_IN_BUF_CCSDS(max_packets_in_buffer), MAX_IN_BUF_VCID(max_vcdu_in_buffer), MPDU_TSIZE(mpdu_tsize), MPDU_DSIZE(mpdu_dsize)
    {
        for (int i = 0; i < 64; i++)
        {
            vcid_queues[i] = new rigtorp::MPMCQueue<ccsds::CCSDSPacket>(MAX_IN_BUF_CCSDS);
        }
    }

    ~CCSDSMuxer()
    {
        for (int i = 0; i < 64; i++)
        {
            delete vcid_queues[i];
        }
    }

    bool pushPacket(int vcid, ccsds::CCSDSPacket &pkt);
    void work(uint8_t *vcdus, int cnt); // Generate the requested amount of CADUs. If no data is present, fill will be added.
};