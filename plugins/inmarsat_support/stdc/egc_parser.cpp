#include "egc_parser.h"
#include "logger.h"
#include "packets_structs.h"

namespace inmarsat
{
    namespace stdc
    {
        egc_t EGCMessageParser::parse_to_msg(nlohmann::json &msg)
        {
            egc_t v;
            v.pkt = msg;
            v.is_p2 = get_packet_frm_id(msg) == pkts::PacketEGCDoubleHeader2::FRM_ID;
            v.msg_id = msg["message_sequence_number"].get<int>();
            v.pkt_no = msg["packet_sequence_number"].get<int>();
            v.timestamp = msg["timestamp"].get<double>();
            v.message = msg["message"].get<std::string>();
            return v;
        }

        nlohmann::json EGCMessageParser::serialize_from_msg(egc_t m, std::string ms)
        {
            nlohmann::json v;
            v = m.pkt;
            v["message"] = ms;
            if (v.contains("packet_sequence_number"))
                v.erase("packet_sequence_number");
            return v;
        }

        void EGCMessageParser::push_message(nlohmann::json msg)
        {
            egc_t m = parse_to_msg(msg);

            // if (wip_messages.count(m.msg_id) == 0)
            //     wip_messages.insert({m.msg_id, std::vector<egc_t>()});

            if (m.msg_id != current_id)
            {
                if (current_id != -1)
                {
                    std::string final_msg;
                    for (auto &mp : wip_message)
                        final_msg += mp.message;
                    on_message(serialize_from_msg(wip_message[wip_message.size() - 1], final_msg));
                    wip_message.clear();
                }

                current_id = m.msg_id;
            }

            bool has_id = false;
            for (auto d : wip_message)
                if (d.pkt_no == m.pkt_no && d.is_p2 == m.is_p2)
                    has_id = true;

            if (!has_id)
                wip_message.push_back(m);

            std::sort(wip_message.begin(), wip_message.end(), [](const egc_t &v1, const egc_t &v2)
                      { return v1.pkt_no < v2.pkt_no && v2.is_p2; });
        }

        void EGCMessageParser::force_finish()
        {
            if (current_id != -1)
            {
                std::string final_msg;
                for (auto &mp : wip_message)
                    final_msg += mp.message;
                on_message(serialize_from_msg(wip_message[wip_message.size() - 1], final_msg));
                wip_message.clear();
            }
        }

        /*
        void EGCMessageParser::check_curr_messages()
        {
        recheck:
            for (auto &el : wip_messages)
            {
                double diff = current_time - el.second[el.second.size() - 1].timestamp;
                if (diff > 30)
                {
                    std::string final_msg;
                    for (auto &mp : el.second)
                        final_msg += mp.message;
                    on_message(serialize_from_msg(el.second[el.second.size() - 1], final_msg));
                    wip_messages.erase(el.first);
                    goto recheck;
                }
            }
        }

        void EGCMessageParser::push_current_time(double current_time)
        {
            this->current_time = current_time;
            check_curr_messages();
        }
        */
    }
}