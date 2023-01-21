#pragma once

#include <cstdint>
#include "nlohmann/json.hpp"
#include <functional>

namespace inmarsat
{
    namespace stdc
    {
        uint16_t compute_crc(uint8_t *buf, int size);
        std::string get_id_name(uint8_t id);
        std::string get_sat_name(int sat);
        std::string get_les_name(int sat, int lesId);
        nlohmann::json get_services_short(uint8_t is8);
        nlohmann::json get_services(int iss);
        nlohmann::json get_stations(uint8_t *data, int stationCount);
        std::string buf_to_hex_str(uint8_t *buf, int len);
        bool is_binary(std::vector<uint8_t> data, bool checkAll);
        std::string message_to_string(std::vector<uint8_t> buf, int presentation);
        std::string get_service_code_and_address_name(int code);
        std::string get_priority(int priority);
        int get_address_length(int messageType);

        class STDPacketParser
        {
        private:
            nlohmann::json output_meta;

            bool wip_multi_frame_has_start = false;
            int wip_multi_frame_gotten_size = 0;
            std::vector<uint8_t> wip_multi_frame_pkt;

        private:
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