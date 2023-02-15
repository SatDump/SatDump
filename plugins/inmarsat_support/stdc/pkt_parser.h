#pragma once

#include <cstdint>
#include "nlohmann/json.hpp"
#include <functional>

#include "packets_structs.h"

namespace inmarsat
{
    namespace stdc
    {
        class STDPacketParser
        {
        private:
            nlohmann::json output_meta;

            bool wip_multi_frame_has_start = false;
            int wip_multi_frame_gotten_size = 0;
            std::vector<uint8_t> wip_multi_frame_pkt;

        private:
#if 0
            void parse_pkt_27(uint8_t *pkt, nlohmann::json &output);
            void parse_pkt_2a(uint8_t *pkt, nlohmann::json &output);
            void parse_pkt_08(uint8_t *pkt, nlohmann::json &output);
            void parse_pkt_6c(uint8_t *pkt, nlohmann::json &output);
            void parse_pkt_7d(uint8_t *pkt, nlohmann::json &output);
            void parse_pkt_81(uint8_t *pkt, nlohmann::json &output);
            void parse_pkt_83(uint8_t *pkt, nlohmann::json &output);
            void parse_pkt_91(uint8_t *pkt, nlohmann::json &output);
            void parse_pkt_92(uint8_t *pkt, nlohmann::json &output);
            void parse_pkt_9a(uint8_t *pkt, nlohmann::json &output);
            void parse_pkt_a0(uint8_t *pkt, nlohmann::json &output);
            void parse_pkt_a3(uint8_t *pkt, int pkt_len, nlohmann::json &output);
            void parse_pkt_a8(uint8_t *pkt, int pkt_len, nlohmann::json &output);
            void parse_pkt_aa(uint8_t *pkt, int pkt_len, nlohmann::json &output);
            void parse_pkt_ab(uint8_t *pkt, nlohmann::json &output);
            void parse_pkt_ac(uint8_t *pkt, nlohmann::json &output);
            void parse_pkt_ad(uint8_t *pkt, nlohmann::json &output);
            void parse_pkt_b1(uint8_t *pkt, int pkt_len, nlohmann::json &output);
            void parse_pkt_b2(uint8_t *pkt, int pkt_len, nlohmann::json &output);
#endif
            void parse_pkt_bd(uint8_t *pkt, int pkt_len, nlohmann::json &output);
            void parse_pkt_be(uint8_t *pkt, int pkt_len, nlohmann::json &output);

        public:
            void parse_main_pkt(uint8_t *main_pkt, int main_pkt_len = 640);

        public:
            // Callback for all packets
            std::function<void(nlohmann::json)> on_packet = [](nlohmann::json) {};
        };
    }
}