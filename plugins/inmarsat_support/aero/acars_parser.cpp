#include "acars_parser.h"
#include "logger.h"
#include <algorithm>

namespace inmarsat
{
    namespace aero
    {
        namespace acars
        {
            bool is_acars_data(std::vector<uint8_t> &payload)
            {
                return payload.size() > 16 && payload[0] == 0xFF && payload[1] == 0xFF;
            }

            // Yes, I did end up peeking in JAERO a bit to figure this
            // part out. 0 documentation apparently!
            ACARSPacket::ACARSPacket(std::vector<uint8_t> pkt)
            {
                has_text = false;
                more_to_come = false;

                mode = pkt[3] & 0x7F;
                tak = (char)(pkt[11] & 0x7F);
                label += (char)(pkt[12] & 0x7F);
                label += (char)(pkt[13] & 0x7F);
                bi = (char)(pkt[14] & 0x7F);

                if (pkt[pkt.size() - 4] == 0x97)
                    more_to_come = true;

                std::vector<int> char_parity;

                for (int i = 0; i < (int)pkt.size(); i++)
                {
                    uint8_t b = pkt[i];
                    int parity = 0;
                    for (int k = 0; k < 8; k++)
                        parity += (b >> k) & 1;
                    char_parity.push_back(parity & 1);
                }

                for (int k = 4; k < 11; k++)
                {
                    if (!char_parity[k])
                        throw std::runtime_error("Acars Text Parity Error");
                    plane_reg += pkt[k] & 0x7F;
                }

                if (pkt[15] == 0x02)
                {
                    has_text = true;

                    for (int k = 16; k < (int)pkt.size() - 4; k++)
                    {
                        if (!char_parity[k])
                            throw std::runtime_error("Acars Text Parity Error");
                        char ch = pkt[k] & 0x7F;
                        if (ch == 0x7F)
                            message += "<DEL>";
                        else
                            message += ch;
                    }
                }
            }

            std::optional<ACARSPacket> ACARSParser::parse(std::vector<uint8_t> &payload)
            {
                auto acar = ACARSPacket(payload);

                if (acar.more_to_come)
                {
                    if (pkt_series.size() > 0)
                        if (pkt_series.begin()->plane_reg != acar.plane_reg)
                            pkt_series.clear();

                    pkt_series.push_back(acar);

                    return std::optional<ACARSPacket>();
                }
                else
                {
                    if (pkt_series.size() > 0)
                    {
                        if (pkt_series.begin()->plane_reg == acar.plane_reg)
                        {
                            ACARSPacket finpkt = acar;
                            finpkt.message.clear();
                            for (auto p : pkt_series)
                                finpkt.message += p.message;
                            return finpkt;
                        }
                    }

                    return acar;
                }
            }

            nlohmann::json parse_libacars(ACARSPacket &pkt, la_msg_dir dir)
            {
                la_proto_node *node = la_acars_decode_apps(pkt.label.data(), pkt.message.data(), dir);
                if (node != NULL)
                {
                    la_vstring *str = la_proto_tree_format_json(NULL, node);
                    auto v = nlohmann::json::parse(std::string(str->str));
                    la_vstring_destroy(str, true);
                    return v;
                }
                la_proto_tree_destroy(node);
                return nlohmann::json();
            }
        }
    }
}