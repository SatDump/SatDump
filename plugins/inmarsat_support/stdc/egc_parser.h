#pragma once

#include "nlohmann/json.hpp"

namespace inmarsat
{
    namespace stdc
    {
        struct egc_t
        {
            nlohmann::json pkt;
            bool is_p2;
            int msg_id;
            int pkt_no;
            double timestamp;
            bool is_cont;
            std::string message;
        };

        class EGCMessageParser
        {
        private:
            egc_t parse_to_msg(nlohmann::json &msg);
            // void check_curr_messages();
            nlohmann::json serialize_from_msg(egc_t m, std::string ms);

        private:
            std::map<int, std::vector<egc_t>> wip_messages;

        public:
            void push_message(nlohmann::json msg);
            // void push_current_time(double current_time);
            void force_finish();

        public:
            // Callback for finished messages
            std::function<void(nlohmann::json)> on_message = [](nlohmann::json) {};
        };
    }
}