#pragma once
#include <string>
#include <vector>
#include <variant>
#include "nlohmann/json.hpp"

namespace goes
{
    namespace hrit
    {
        struct PEInfo
        {
            std::string name;
            int record_interval = 0;
            int read_to_transmit_offset = 0;
            float base_elevation = 0.0f;
            float correction = 0.0f;
        };

        struct DCP
        {
            std::vector<PEInfo> pe_info;
            char data_format = 0;
            std::string agency;
            std::string state;
            std::string manufacturer;
            std::string model;
            std::string nmc_descriptor;
            std::string description;
            int transmit_interval = -1;
            int transmit_window = -1;
            float lat = 0.0f;
            float lon = 0.0f;
        };

        struct DCSMessageHeader
        {
            bool crc_pass = false;
            uint32_t sequence_number = 0;
            std::string data_rate;
            std::string cs_platform;

            bool parity_errors = false;
            bool no_eot = false;
            bool address_corrected = false;
            bool uncorrectable_address = false;
            bool invalid_address = false;
            bool pdt_incomplete = false;
            bool timing_error = false;
            bool unexpected_message = false;
            bool wrong_channel = false;

            std::string corrected_address;
            std::string carrier_start;
            std::string message_end;

            float signal_strength = 0.0f;
            float freq_offset = 0.0f;
            float phase_noise = 0.0f;
            float good_phase = 0.0f;

            uint16_t channel = 0;

            std::string modulation_index;
            std::string spacecraft;
            std::string drgs_source;

            int type_id = 0; // Technically not part of a header, but the first word of the body
        };

        struct MissedMessageHeader
        {
            bool crc_pass = false;
            uint32_t sequence_number = 0;
            uint16_t channel = 0;
            std::string data_rate;
            std::string platform_address;
            std::string window_start;
            std::string window_end;
            std::string spacecraft;
        };

        struct DCSValue
        {
            std::string name;
            int interval = 0;
            int reading_age = 0;
            std::vector<std::string> values;
        };

        struct DCSMessage
        {
            std::string type = "DCS Message";
            DCSMessageHeader header;
            std::string data_type = "Unknown";
            std::shared_ptr<DCP> dcp = nullptr;
            std::string data_raw;
            std::string data_ascii;
            std::vector<DCSValue> data_values;
        };

        struct MissedMessage
        {
            std::string type = "Missed Message";
            MissedMessageHeader header;
            std::shared_ptr<DCP> dcp = nullptr;
        };

        struct DCSFile
        {
            std::string name;
            std::string source;
            std::string type;
            // std::string exp_field;

            bool header_crc_pass = false;
            bool file_crc_pass = false;

            std::vector<std::variant<std::shared_ptr<DCSMessage>, std::shared_ptr<MissedMessage>>> blocks;
        };

        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PEInfo, name, record_interval, read_to_transmit_offset, base_elevation, correction);
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DCP, pe_info, agency, state, manufacturer, model, nmc_descriptor, description, transmit_interval,
            transmit_window, lat, lon, data_format);
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DCSMessageHeader, crc_pass, sequence_number, data_rate, cs_platform,
            parity_errors, no_eot, address_corrected, uncorrectable_address, invalid_address, pdt_incomplete,
            timing_error, unexpected_message, wrong_channel, corrected_address, carrier_start, message_end,
            signal_strength, freq_offset, phase_noise, good_phase, channel, modulation_index, spacecraft,
            drgs_source, type_id);
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MissedMessageHeader, crc_pass, sequence_number, channel, data_rate,
            platform_address, window_start, window_end, spacecraft);
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DCSValue, name, reading_age, interval, values);

        inline void to_json(nlohmann::json &j, const DCSMessage &v)
        {
            j["type"] = v.type;
            j["header"] = v.header;
            j["data_type"] = v.data_type;
            j["data_raw"] = v.data_raw;
            j["data_ascii"] = v.data_ascii;
            j["data_values"] = v.data_values;

            if (v.dcp == nullptr)
                j["dcp"] = nullptr;
            else
                j["dcp"] = *v.dcp;
        }

        inline void from_json(const nlohmann::json &j, DCSMessage &v)
        {
            j.at("type").get_to(v.type);
            j.at("header").get_to(v.header);

            if (j.at("dcp").is_null())
                v.dcp = nullptr;
            else
                v.dcp = std::make_shared<DCP>((DCP)(j.at("dcp")));
        };

        inline void to_json(nlohmann::json &j, const MissedMessage &v)
        {
            j["type"] = v.type;
            j["header"] = v.header;
            if (v.dcp == nullptr)
                j["dcp"] = nullptr;
            else
                j["dcp"] = *v.dcp;
        }

        inline void from_json(const nlohmann::json &j, MissedMessage &v)
        {
            j.at("type").get_to(v.type);
            j.at("header").get_to(v.header);

            if (j.at("dcp").is_null())
                v.dcp = nullptr;
            else
                v.dcp = std::make_shared<DCP>((DCP)(j.at("dcp")));
        };

        inline void to_json(nlohmann::ordered_json &j, const DCSFile &v)
        {
            j["name"] = v.name;
            j["source"] = v.source;
            j["type"] = v.type;
            j["header_crc_pass"] = v.header_crc_pass;
            j["file_crc_pass"] = v.file_crc_pass;

            nlohmann::json blocks_arr = nlohmann::json::array();
            for (auto& block : v.blocks)
                std::visit([&blocks_arr](const auto &v) { blocks_arr.push_back(*v); }, block);
            j["blocks"] = blocks_arr;
        }

        inline void from_json(const nlohmann::ordered_json &j, DCSFile &v)
        {
            j.at("name").get_to(v.name);
            j.at("source").get_to(v.source);
            j.at("type").get_to(v.type);
            j.at("header_crc_pass").get_to(v.header_crc_pass);
            j.at("file_crc_pass").get_to(v.file_crc_pass);

            for (auto& block : j["blocks"])
            {
                if (block["type"] == "DCS Message")
                    v.blocks.push_back(std::make_shared<DCSMessage>(block.get<DCSMessage>()));
                else if (block["type"] == "Missed Message")
                    v.blocks.push_back(std::make_shared<MissedMessage>(block.get<MissedMessage>()));
            }
        };
    }
}