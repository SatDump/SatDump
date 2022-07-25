#include "dvb_mpe.h"

namespace mpeg_ts
{
    void MPEHeader::parse(uint8_t *frm)
    {
        table_id = frm[0];
        section_syntax_indicator = frm[1] >> 7;
        private_indicator = (frm[1] >> 6) & 1;
        section_length = (frm[1] & 0x0F) << 8 | frm[2];
        mac_address_6 = frm[3];
        mac_address_5 = frm[4];
        payload_scrambling_control = (frm[5] >> 4) & 0b11;
        address_scrambling_control = (frm[5] >> 2) & 0b11;
        llc_snap_flag = (frm[5] >> 1) & 1;
        current_next_indicator = frm[5] & 1;
        section_number = frm[6];
        last_section_number = frm[7];
        mac_address_4 = frm[8];
        mac_address_3 = frm[9];
        mac_address_2 = frm[10];
        mac_address_1 = frm[11];
    }

    void IPv4Header::parse(uint8_t *frm)
    {
        version = frm[0] >> 4;
        ihl = frm[1] & 0x0F;
        dscp = frm[2] >> 2;
        ecn = frm[2] & 0b11;
        total_length = frm[3] << 8 | frm[4];
        idenditication = frm[5] << 8 | frm[6];
        flags = frm[7] >> 5;
        fragment_offset = (frm[7] & 0b11111) << 8 | frm[8];
        time_to_live = frm[9];
        protocol = frm[10];
        header_checksum = frm[22] << 8 | frm[11];
        source_ip_1 = frm[12];
        source_ip_2 = frm[13];
        source_ip_3 = frm[14];
        source_ip_4 = frm[15];
        target_ip_1 = frm[16];
        target_ip_2 = frm[17];
        target_ip_3 = frm[18];
        target_ip_4 = frm[19];
    }
};