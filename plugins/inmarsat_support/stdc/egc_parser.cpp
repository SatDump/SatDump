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
            v.is_cont = msg["continuation"].get<bool>();
            return v;
        }

        nlohmann::json EGCMessageParser::serialize_from_msg(egc_t m, std::string ms)
        {
            nlohmann::json v;
            v = m.pkt;
            v["message"] = ms;
            if (v.contains("packet_sequence_number"))
                v.erase("packet_sequence_number");
            if (v.contains("data"))
                v.erase("data");
            return v;
        }

        void EGCMessageParser::push_message(nlohmann::json msg)
        {
            egc_t m = parse_to_msg(msg);

            if (wip_messages.count(m.msg_id) == 0)
                wip_messages.insert({m.msg_id, std::vector<egc_t>()});

            bool has_id = false;
            for (auto d : wip_messages[m.msg_id])
                if (d.pkt_no == m.pkt_no && d.is_p2 == m.is_p2)
                    has_id = true;

            if (!has_id)
                wip_messages[m.msg_id].push_back(m);

            std::sort(wip_messages[m.msg_id].begin(), wip_messages[m.msg_id].end(), [](const egc_t &v1, const egc_t &v2)
                      { return (v1.pkt_no * 2 + v1.is_p2) < (v2.pkt_no * 2 + v2.is_p2); });

            if (m.is_p2 && !m.is_cont)
            {
                auto &fmsg = wip_messages[m.msg_id];
                if (fmsg.size() > 0)
                {
                    std::string final_msg;
                    for (auto &mp : fmsg)
                        final_msg += mp.message;
                    on_message(serialize_from_msg(fmsg[fmsg.size() - 1], final_msg));
                    wip_messages.erase(m.msg_id);
                }
            }
        }

        void EGCMessageParser::force_finish()
        {
            for (auto &fmsg : wip_messages)
            {
                if (fmsg.second.size() > 0)
                {
                    std::string final_msg;
                    for (auto &mp : fmsg.second)
                        final_msg += mp.message;
                    on_message(serialize_from_msg(fmsg.second[fmsg.second.size() - 1], final_msg));
                }
                fmsg.second.clear();
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