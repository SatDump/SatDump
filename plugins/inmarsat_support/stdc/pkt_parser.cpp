#include "pkt_parser.h"
#include "logger.h"

namespace inmarsat
{
    namespace stdc
    {
        void STDPacketParser::parse_pkt_bd(uint8_t *pkt, int pkt_len, nlohmann::json &)
        {
            uint8_t mid = pkt[2] & 0xFF;

            int mpkt_len = 0;
            if (mid >> 7 == 0)
                mpkt_len = (mid & 0x0F) + 1;
            else if (mid >> 6 == 0x02)
                mpkt_len = pkt[3] + 2;

            wip_multi_frame_pkt.clear();
            wip_multi_frame_pkt.resize(mpkt_len, 0);

            wip_multi_frame_gotten_size = pkt_len - 2 - 2;
            memcpy(&wip_multi_frame_pkt[0], &pkt[2], wip_multi_frame_gotten_size);

            wip_multi_frame_has_start = true;
        }

        void STDPacketParser::parse_pkt_be(uint8_t *pkt, int pkt_len, nlohmann::json &)
        {
            if (!wip_multi_frame_has_start)
                return;
            int actual_length = pkt_len - 2 - 2;
            memcpy(&wip_multi_frame_pkt[wip_multi_frame_gotten_size], &pkt[2], actual_length);
            wip_multi_frame_gotten_size += actual_length;
        }

        void STDPacketParser::parse_main_pkt(uint8_t *main_pkt, int main_pkt_len)
        {
            double timestamp = time(0);

            int pos = 0;
            while (pos < main_pkt_len)
            {
                // Setup utils vars
                uint8_t *pkt = &main_pkt[pos];
                int pkt_len_max = main_pkt_len - pos;

                if (pkt[0] == 0x00) // No more packets
                    return;

                pkts::PacketBase pkt_b(&pkt[0], pkt_len_max);

                // Clear output
                output_meta.clear();

                switch (pkt_b.descriptor.type)
                {
                case pkts::PacketBulletinBoard::FRM_ID:
                    output_meta = pkts::PacketBulletinBoard(pkt, pkt_len_max);
                    break;
                case pkts::PacketSignallingChannel::FRM_ID:
                    output_meta = pkts::PacketSignallingChannel(pkt, pkt_len_max);
                    break;
                case pkts::PacketAcknowledgement::FRM_ID:
                    output_meta = pkts::PacketAcknowledgement(pkt, pkt_len_max);
                    break;
                case pkts::PacketAcknowledgementRequest::FRM_ID:
                    output_meta = pkts::PacketAcknowledgementRequest(pkt, pkt_len_max);
                    break;
                case pkts::PacketAnnouncement::FRM_ID:
                    output_meta = pkts::PacketAnnouncement(pkt, pkt_len_max);
                    break;
                case pkts::PacketLESForcedClear::FRM_ID:
                    output_meta = pkts::PacketLESForcedClear(pkt, pkt_len_max);
                    break;
                case pkts::PacketClear::FRM_ID:
                    output_meta = pkts::PacketClear(pkt, pkt_len_max);
                    break;
                case pkts::PacketConfirmation::FRM_ID:
                    output_meta = pkts::PacketConfirmation(pkt, pkt_len_max);
                    break;
                case pkts::PacketDistressAlertAcknowledgement::FRM_ID:
                    output_meta = pkts::PacketDistressAlertAcknowledgement(pkt, pkt_len_max);
                    break;
                case pkts::PacketDistressTestRequest::FRM_ID:
                    output_meta = pkts::PacketDistressTestRequest(pkt, pkt_len_max);
                    break;
                case pkts::PacketEGCSingleHeader::FRM_ID:
                    output_meta = pkts::PacketEGCSingleHeader(pkt, pkt_len_max);
                    break;
                case pkts::PacketEGCDoubleHeader1::FRM_ID:
                    output_meta = pkts::PacketEGCDoubleHeader1(pkt, pkt_len_max);
                    break;
                case pkts::PacketEGCDoubleHeader2::FRM_ID:
                    output_meta = pkts::PacketEGCDoubleHeader2(pkt, pkt_len_max);
                    break;
                case pkts::PacketLogicalChannelAssignement::FRM_ID:
                    output_meta = pkts::PacketLogicalChannelAssignement(pkt, pkt_len_max);
                    break;
                case pkts::PacketLoginAcknowledgment::FRM_ID:
                    output_meta = pkts::PacketLoginAcknowledgment(pkt, pkt_len_max);
                    break;
                case pkts::PacketLogoutAcknowledgment::FRM_ID:
                    output_meta = pkts::PacketLogoutAcknowledgment(pkt, pkt_len_max);
                    break;
                case pkts::PacketMessageData::FRM_ID:
                    output_meta = pkts::PacketMessageData(pkt, pkt_len_max);
                    break;
                case pkts::PacketMessageStatus::FRM_ID:
                    output_meta = pkts::PacketMessageStatus(pkt, pkt_len_max);
                    break;
                case pkts::PacketNetworkUpdate::FRM_ID:
                    output_meta = pkts::PacketNetworkUpdate(pkt, pkt_len_max);
                    break;
                case pkts::PacketRequestStatus::FRM_ID:
                    output_meta = pkts::PacketRequestStatus(pkt, pkt_len_max);
                    break;
                case pkts::PacketTestResult::FRM_ID:
                    output_meta = pkts::PacketTestResult(pkt, pkt_len_max);
                    break;
                case pkts::PacketNetworkMonitor::FRM_ID:
                    output_meta = pkts::PacketNetworkMonitor(pkt, pkt_len_max);
                    break;
                    ///////////////////////////////////////////////
                case 0x3d: // Multiframe start
                    parse_pkt_bd(pkt, pkt_b.descriptor.length, output_meta);
                    break;
                case 0x3e: // Multiframe end
                    parse_pkt_be(pkt, pkt_b.descriptor.length, output_meta);

                    // Check packet is usable
                    if (wip_multi_frame_has_start && (wip_multi_frame_gotten_size == (int)wip_multi_frame_pkt.size() - 2))
                    {
                        STDPacketParser dec;
                        dec.on_packet = on_packet;
                        dec.parse_main_pkt(wip_multi_frame_pkt.data(), wip_multi_frame_pkt.size());
                    }

                    wip_multi_frame_has_start = false;
                    wip_multi_frame_gotten_size = 0;
                    wip_multi_frame_pkt.clear();

                    break;
                default:
                    output_meta["descriptor"] = pkt_b.descriptor;
                }

                // If this is 0x7D and first frame, use better timestamp
                if (pkt_b.descriptor.type == pkts::PacketBulletinBoard::FRM_ID && pos == 0)
                {
                    time_t currentDay = time(0);
                    time_t dayValue = currentDay - (currentDay % 86400);
                    timestamp = dayValue + output_meta.get<pkts::PacketBulletinBoard>().seconds_of_day;
                }

                // Timestamp packet
                output_meta["timestamp"] = timestamp + ((double)pos / (double)main_pkt_len) * 8.64;

                // Not a multiframe PKT, process it now.
                if (pkt_b.descriptor.type != 0x3d && pkt_b.descriptor.type != 0x3e)
                {
                    on_packet(output_meta);
                    output_meta.clear();
                }

                pos += pkt_b.descriptor.length;
            }
        }
    }
}