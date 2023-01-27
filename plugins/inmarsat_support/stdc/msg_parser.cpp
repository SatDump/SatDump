#include "msg_parser.h"
#include "logger.h"

namespace inmarsat
{
    namespace stdc
    {
        msg_t MessageParser::parse_to_msg(nlohmann::json &msg)
        {
            msg_t v;
            v.pkt = msg;
            v.logical_channel = msg["logical_channel_number"].get<int>();
            v.pkt_no = msg["packet_sequence_number"].get<int>();
            v.timestamp = msg["timestamp"].get<double>();
            v.message = msg["message"].get<std::string>();
            return v;
        }

        nlohmann::json MessageParser::serialize_from_msg(msg_t m, std::string ms)
        {
            nlohmann::json v;
            v = m.pkt;
            v["message"] = ms;
            if (v.contains("packet_sequence_number"))
                v.erase("packet_sequence_number");
            return v;
        }

        void MessageParser::push_message(nlohmann::json msg)
        {
            msg_t m = parse_to_msg(msg);

            if (wip_messages.count(m.logical_channel) == 0)
                wip_messages.insert({m.logical_channel, std::vector<msg_t>()});

            wip_messages[m.logical_channel].push_back(m);

            std::sort(wip_messages[m.logical_channel].begin(), wip_messages[m.logical_channel].end(), [](const msg_t &v1, const msg_t &v2)
                      { return v1.pkt_no < v2.pkt_no; });
        }

        void MessageParser::check_curr_messages()
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

        void MessageParser::push_current_time(double current_time)
        {
            this->current_time = current_time;
            check_curr_messages();
        }
    }
}