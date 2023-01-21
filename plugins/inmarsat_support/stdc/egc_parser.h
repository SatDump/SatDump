#pragma once

#include "nlohmann/json.hpp"

namespace inmarsat
{
    namespace stdc
    {
        struct egc_t
        {
            nlohmann::json pkt;
            int msg_id;
            int pkt_no;
            double timestamp;
            std::string message;
        };

        class EGCMessageParser
        {
        private:
            egc_t parse_to_msg(nlohmann::json &msg);
            void check_curr_messages();
            nlohmann::json serialize_from_msg(egc_t m, std::string ms);

        private:
            double current_time;
            std::map<int, std::vector<egc_t>> wip_messages;
            // std::vector<egc_t> wip_message;

        public:
            void push_message(nlohmann::json msg);
            void push_current_time(double current_time);

        public:
            // Callback for finished messages
            std::function<void(nlohmann::json)> on_message = [](nlohmann::json) {};
        };
    }
}