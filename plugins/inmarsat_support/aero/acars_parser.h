#pragma once

#include <cstdint>
#include <string>
#include "nlohmann/json.hpp"
#include "pkt_structs.h"
#include <optional>

#include "libacars/libacars.h"
#include "libacars/acars.h"
#include "libacars/vstring.h"

namespace inmarsat
{
    namespace aero
    {
        namespace acars
        {
            bool is_acars_data(std::vector<uint8_t> &payload);

            struct ACARSPacket
            {
                uint8_t mode;
                uint8_t tak;
                std::string label = "";
                uint8_t bi;
                std::string plane_reg = "";

                bool has_text;
                std::string message;

                bool more_to_come;

                ACARSPacket() {}
                ACARSPacket(std::vector<uint8_t> pkt);

                NLOHMANN_DEFINE_TYPE_INTRUSIVE(ACARSPacket,
                                               mode, tak, label, bi,
                                               plane_reg, has_text, message, more_to_come)
            };

            class ACARSParser
            {
            private:
                std::vector<ACARSPacket> pkt_series;

            public:
                std::optional<ACARSPacket> parse(std::vector<uint8_t> &payload);
            };

            nlohmann::json parse_libacars(ACARSPacket &pkt, la_msg_dir dir);
        }
    }
}