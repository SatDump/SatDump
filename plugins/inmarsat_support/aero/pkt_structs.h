#pragma once

#include <cstdint>
#include <string>
#include "nlohmann/json.hpp"

namespace inmarsat
{
    namespace aero
    {
        std::string pkt_type_to_name(uint8_t id);
        bool check_crc(uint8_t *pkt);

        namespace pkts
        {
            using namespace ::inmarsat::aero;

            // Common packet format :
            // - Pkt Type
            // - Data field
            // - Checksum
            struct MessageBase
            {
                uint8_t message_type;

                MessageBase() {}
                MessageBase(uint8_t *pkt)
                {
                    message_type = pkt[0];
                }

                // NLOHMANN_DEFINE_TYPE_INTRUSIVE(MessageBase, descriptor)
            };

            struct MessageAESSystemTableBroadcastIndex : public MessageBase
            {
                static const uint8_t MSG_ID = 0x0A;

                uint8_t revision_number;

                uint8_t initial_seq_no_of_a2_31_partial;
                uint8_t initial_seq_no_of_a2_32_33_partial;
                uint8_t initial_seq_no_of_a2_34_partial;
                uint8_t initial_seq_no_of_a2_31_complete;
                uint8_t initial_seq_no_of_a2_32_33_complete;
                uint8_t initial_seq_no_of_a2_34_complete;
                bool has_eirp_table_complete;
                bool has_eirp_table_partial;
                bool has_spot_beam_series;

                MessageAESSystemTableBroadcastIndex() {}
                MessageAESSystemTableBroadcastIndex(uint8_t *pkt) : MessageBase(pkt)
                {
                    revision_number = pkt[1];

                    initial_seq_no_of_a2_31_partial = pkt[2] >> 2;
                    initial_seq_no_of_a2_32_33_partial = pkt[3] >> 2;
                    initial_seq_no_of_a2_34_partial = pkt[4] >> 2;

                    initial_seq_no_of_a2_31_complete = pkt[5] >> 2;
                    initial_seq_no_of_a2_32_33_complete = pkt[6] >> 2;
                    initial_seq_no_of_a2_34_complete = pkt[7] >> 2;

                    has_eirp_table_complete = (pkt[2] >> 1) & 1;
                    has_eirp_table_partial = (pkt[3] >> 1) & 1;
                    has_spot_beam_series = pkt[9] & 1;
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(MessageAESSystemTableBroadcastIndex, message_type,
                                               initial_seq_no_of_a2_31_partial, initial_seq_no_of_a2_32_33_partial, initial_seq_no_of_a2_34_partial,
                                               initial_seq_no_of_a2_31_complete, initial_seq_no_of_a2_32_33_complete, initial_seq_no_of_a2_34_complete,
                                               has_eirp_table_complete, has_eirp_table_partial, has_spot_beam_series)
            };

            struct MessageUserDataISU : public MessageBase
            {
                static const uint8_t MSG_ID = 0x71;

                uint32_t aes_id;
                uint8_t ges_id;

                uint8_t q_no;
                uint8_t ref_no;
                uint8_t seq_no;

                uint8_t no_of_bytes_in_last_su;

                std::vector<uint8_t> user_data;

                MessageUserDataISU() {}
                MessageUserDataISU(uint8_t *pkt) : MessageBase(pkt)
                {
                    aes_id = pkt[1] << 16 | pkt[2] << 8 | pkt[3];
                    ges_id = pkt[4];
                    q_no = pkt[5] >> 4;
                    ref_no = pkt[5] & 0xF;
                    seq_no = pkt[6] & 0x3F;
                    no_of_bytes_in_last_su = pkt[7] >> 4;

                    user_data.insert(user_data.end(), &pkt[8], &pkt[10]);
                }

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(MessageUserDataISU, message_type,
                                               aes_id, ges_id, q_no, ref_no, seq_no,
                                               no_of_bytes_in_last_su)
            };

            struct MessageUserDataSSU : public MessageBase
            {
                // static const uint8_t MSG_ID = 0xNA;

                uint8_t seq_no;
                uint8_t q_no;
                uint8_t ref_no;

                std::vector<uint8_t> user_data;

                MessageUserDataSSU() {}
                MessageUserDataSSU(uint8_t *pkt) : MessageBase(pkt)
                {
                    seq_no = pkt[0] & 0x3F;
                    q_no = pkt[1] >> 4;
                    ref_no = pkt[1] & 0xF;

                    user_data.insert(user_data.end(), &pkt[2], &pkt[10]);
                }
            };

            struct MessageUserDataFinal
            {
                MessageUserDataISU isu;
                std::vector<MessageUserDataSSU> ssu;

                std::vector<uint8_t> get_payload()
                {
                    std::vector<uint8_t> ret;

                    // ISU
                    ret.insert(ret.end(), isu.user_data.begin(), isu.user_data.end());

                    // SSU (without last)
                    for (int i = 0; i < (int)ssu.size() - 1; i++)
                        ret.insert(ret.end(), ssu[i].user_data.begin(), ssu[i].user_data.end());

                    int size_of_last_su = isu.no_of_bytes_in_last_su;
                    if (size_of_last_su > 8)
                        size_of_last_su = 8;

                    int i = ssu.size() - 1;
                    ret.insert(ret.end(), ssu[i].user_data.begin(), ssu[i].user_data.begin() + size_of_last_su);

                    return ret;
                }
            };
        }
    }
}