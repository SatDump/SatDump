#include "packets_structs.h"
#include <sstream>

namespace inmarsat
{
    namespace stdc
    {
        std::string get_id_name(uint8_t id)
        {
            if (id == 0x00)
                return "Acknowledgement Request";
            else if (id == 0x01)
                return "Announcement";
            else if (id == 0x02)
                return "Logical Channel Clear";
            else if (id == 0x03)
                return "Logical Channel Assignment";
            else if (id == 0x04)
                return "LES TDM Channel Descriptor Packet";
            else if (id == 0x05)
                return "Network Monitor Packet";
            else if (id == 0x06)
                return "Signalling Channel";
            else if (id == 0x07)
                return "Bulletin Board";
            ////////////
            else if (id == 0x10)
                return "Acknowledgement";
            else if (id == 0x11)
                return "Distress Alert Acknowledgement";
            else if (id == 0x12)
                return "Login Acknowledgement";
            else if (id == 0x13)
                return "Logout Acknowledgement";
            ////////////
            if (id == 0x19)
                return "LES Forced Clear";
            else if (id == 0x1a)
                return "Enhanced Data Report Acknowledgement";
            ////////////
            else if (id == 0x20)
                return "Distress Test Request";
            else if (id == 0x21)
                return "Area Poll";
            else if (id == 0x22)
                return "Group Poll";
            else if (id == 0x23)
                return "Individual Poll";
            else if (id == 0x24)
                return "Mobile To Base Station Poll";
            else if (id == 0x25)
                return "Mobile To Mobile Poll";
            ////////////
            else if (id == 0x28)
                return "Confirmation";
            else if (id == 0x29)
                return "Message Status";
            else if (id == 0x2a)
                return "Message Data";
            else if (id == 0x2b)
                return "Network Update";
            else if (id == 0x2c)
                return "Request Status";
            else if (id == 0x2d)
                return "Test Result";
            else if (id == 0x30)
                return "EGC Packet, single header";
            else if (id == 0x31)
                return "EGC double header, part 1";
            else if (id == 0x32)
                return "EGC double header, part 2";
            ////////////
            else if (id == 0x3D)
                return "Multiframe Packet Start";
            else if (id == 0x3E)
                return "Multiframe Packet Continue";
            else
                return "Unknown";
        }

        std::string get_sat_name(int sat)
        {
            if (sat == 0)
                return "Atlantic Ocean Region West (AOR-W)";
            else if (sat == 1)
                return "Atlantic Ocean Region East (AOR-E)";
            else if (sat == 2)
                return "Pacific Ocean Region (POR)";
            else if (sat == 3)
                return "Indian Ocean Region (IOR)";
            else if (sat == 9)
                return "All Ocean Regions Covered by the LES";
            else
                return "Unknown";
        }

        std::string get_les_name(int sat, int lesId)
        {
            int value = lesId + sat * 100;
            std::string name;
            switch (value)
            {
            case 001:
            case 101:
            case 201:
            case 301:
                name = "Vizada-Telenor, USA";
                break;

            case 002:
            case 102:
            case 302:
                name = "Stratos Global (Burum-2), Netherlands";
                break;

            case 202:
                name = "Stratos Global (Aukland), New Zealand";
                break;

            case 003:
            case 103:
            case 203:
            case 303:
                name = "KDDI Japan";
                break;

            case 004:
            case 104:
            case 204:
            case 304:
                name = "Vizada-Telenor, Norway";
                break;

            case 044:
            case 144:
            case 244:
            case 344:
                name = "NCS";
                break;

            case 105:
            case 335:
                name = "Telecom, Italia";
                break;

            case 305:
            case 120:
                name = "OTESTAT, Greece";
                break;

            case 306:
                name = "VSNL, India";
                break;

            case 110:
            case 310:
                name = "Turk Telecom, Turkey";
                break;

            case 211:
            case 311:
                name = "Beijing MCN, China";
                break;

            case 012:
            case 112:
            case 212:
            case 312:
                name = "Stratos Global (Burum), Netherlands";
                break;

            case 114:
                name = "Embratel, Brazil";
                break;

            case 116:
            case 316:
                name = "Telekomunikacja Polska, Poland";
                break;

            case 117:
            case 217:
            case 317:
                name = "Morsviazsputnik, Russia";
                break;

            case 021:
            case 121:
            case 221:
            case 321:
                name = "Vizada (FT), France";
                break;

            case 127:
            case 327:
                name = "Bezeq, Israel";
                break;

            case 210:
            case 328:
                name = "Singapore Telecom, Singapore";
                break;

            case 330:
                name = "VISHIPEL, Vietnam";
                break;

            default:
                name = "Unknown";
                break;
            }
            return std::to_string(value) + ", " + name;
        }

        nlohmann::json get_services_short(uint8_t is8)
        {
            nlohmann::json services;
            services["MaritimeDistressAlerting"] = ((is8 & 0x80) >> 7 == 1);
            services["SafetyNet"] = ((is8 & 0x40) >> 6 == 1);
            services["InmarsatC"] = ((is8 & 0x20) >> 5 == 1);
            services["StoreFwd"] = ((is8 & 0x10) >> 4 == 1);
            services["HalfDuplex"] = ((is8 & 8) >> 3 == 1);
            services["FullDuplex"] = ((is8 & 4) >> 2 == 1);
            services["ClosedNetwork"] = ((is8 & 2) >> 1 == 1);
            services["FleetNet"] = ((is8 & 1));
            return services;
        }

        nlohmann::json get_services(int iss)
        {
            nlohmann::json services;
            services["MaritimeDistressAlerting"] = ((iss & 0x8000) >> 15 == 1);
            services["SafetyNet"] = ((iss & 0x4000) >> 14 == 1);
            services["InmarsatC"] = ((iss & 0x2000) >> 13 == 1);
            services["StoreFwd"] = ((iss & 0x1000) >> 12 == 1);
            services["HalfDuplex"] = ((iss & 0x800) >> 11 == 1);
            services["FullDuplex"] = ((iss & 0x400) >> 10 == 1);
            services["ClosedNetwork"] = ((iss & 0x200) >> 9 == 1);
            services["FleetNet"] = ((iss & 0x100) >> 8 == 1);
            services["PrefixSF"] = ((iss & 0x80) >> 7 == 1);
            services["LandMobileAlerting"] = ((iss & 0x40) >> 6 == 1);
            services["AeroC"] = ((iss & 0x20) >> 5 == 1);
            services["ITA2"] = ((iss & 0x10) >> 4 == 1);
            services["DATA"] = ((iss & 0x08) >> 3 == 1);
            services["BasicX400"] = ((iss & 0x04) >> 2 == 1);
            services["EnhancedX400"] = ((iss & 0x02) >> 1 == 1);
            services["LowPowerCMES"] = ((iss & 0x01) == 1);
            return services;
        }

        nlohmann::json get_stations(uint8_t *data, int stationCount)
        {
            nlohmann::json stations;
            int j = 0;
            for (int i = 0; i < stationCount; i++)
            {
                // stations [": ]+=std::to_string(i) ;
                int sat = data[j] >> 6 & 0x03;
                stations[i]["sa_id"] = sat;
                stations[i]["sat_name"] = get_sat_name(sat);
                int lesId = data[j] & 0x3F;
                stations[i]["les_id"] = lesId;
                stations[i]["les_name"] = get_les_name(sat, lesId);
                int servicesStart = data[j + 1];
                stations[i]["servicesStart"] = servicesStart;
                int iss = data[j + 2] << 8 | data[j + 3];
                // stations ["Services;]
                stations[i]["MaritimeDistressAlerting"] = ((iss & 0x8000) >> 15 == 1);
                stations[i]["SafetyNet"] = ((iss & 0x4000) >> 14 == 1);
                stations[i]["InmarsatC"] = ((iss & 0x2000) >> 13 == 1);
                stations[i]["StoreFwd"] = ((iss & 0x1000) >> 12 == 1);
                stations[i]["HalfDuplex"] = ((iss & 0x800) >> 11 == 1);
                stations[i]["FullDuplex"] = ((iss & 0x400) >> 10 == 1);
                stations[i]["ClosedNetwork"] = ((iss & 0x200) >> 9 == 1);
                stations[i]["FleetNet"] = ((iss & 0x100) >> 8 == 1);
                stations[i]["PrefixSF"] = ((iss & 0x80) >> 7 == 1);
                stations[i]["LandMobileAlerting"] = ((iss & 0x40) >> 6 == 1);
                stations[i]["AeroC"] = ((iss & 0x20) >> 5 == 1);
                stations[i]["ITA2"] = ((iss & 0x10) >> 4 == 1);
                stations[i]["DATA"] = ((iss & 0x08) >> 3 == 1);
                stations[i]["BasicX400"] = ((iss & 0x04) >> 2 == 1);
                stations[i]["EnhancedX400"] = ((iss & 0x02) >> 1 == 1);
                stations[i]["LowPowerCMES"] = ((iss & 0x01) == 1);
                double downlink_mhz = ((data[j + 4] << 8 | data[j + 4]) - 8000) * 0.0025 + 1530.5;
                stations[i]["downlink_channel_mhz "] = (downlink_mhz);
                j += 6;
            }
            return stations;
        }

        bool is_binary(std::vector<uint8_t> data, bool checkAll)
        {
            bool isBinary = false;
            // try first 13 characters if not check all
            int check = 13;
            if (!checkAll)
                check = std::min(check, (int)data.size() - 2);
            else
                check = data.size();
            for (int i = 0; i < check; i++)
            {
                char chr = (char)(data[i] & 0x7F);
                switch (chr)
                {
                case 0x01:
                case 0x05:
                case 0x06:
                case 0x07:
                case 0x08:
                case 0x0B:
                case 0x0C:
                case 0x0E:
                case 0x0F:
                case 0x10:
                case 0x11:
                case 0x12:
                case 0x13:
                case 0x14:
                case 0x15:
                case 0x16:
                case 0x17:
                case 0x18:
                case 0x19:
                case 0x1A:
                case 0x1C:
                case 0x1D:
                case 0x1E:
                case 0x1F:
                // case 0xA4:
                case '$':
                    isBinary = true;
                    break;
                }
            }
            return isBinary;
        }

        std::map<int, char> table_xia5 = {
            //{0x00, 0},
            {10, '\n'},
            {13, '\r'},
            //
            {0x21, '!'},
            {0x22, '"'},
            {0x23, '#'},
            //{0x24, '¤'},
            {0x25, '%'},
            {0x26, '&'},
            {0x27, '\''},
            {0x28, '('},
            {0x29, ')'},
            {0x2A, '*'},
            {0x2B, '+'},
            {0x2C, ','},
            {0x2D, '-'},
            {0x2E, '.'},
            {0x2F, '/'},
            //
            {0x30, '0'},
            {0x31, '1'},
            {0x32, '2'},
            {0x33, '3'},
            {0x34, '4'},
            {0x35, '5'},
            {0x36, '6'},
            {0x37, '7'},
            {0x38, '8'},
            {0x39, '9'},
            {0x3A, ':'},
            {0x3B, ';'},
            {0x3C, '<'},
            {0x3D, '='},
            {0x3E, '>'},
            {0x3F, '?'},
            //
            {0x40, '@'},
            {0x41, 'A'},
            {0x42, 'B'},
            {0x43, 'C'},
            {0x44, 'D'},
            {0x45, 'E'},
            {0x46, 'F'},
            {0x47, 'G'},
            {0x48, 'H'},
            {0x49, 'I'},
            {0x4A, 'J'},
            {0x4B, 'K'},
            {0x4C, 'L'},
            {0x4D, 'M'},
            {0x4E, 'N'},
            {0x4F, 'O'},
            //
            {0x50, 'P'},
            {0x51, 'Q'},
            {0x52, 'R'},
            {0x53, 'S'},
            {0x54, 'T'},
            {0x55, 'U'},
            {0x56, 'V'},
            {0x57, 'W'},
            {0x58, 'X'},
            {0x59, 'Y'},
            {0x5A, 'Z'},
            {0x5B, '['},
            {0x5C, '\\'},
            {0x5D, ']'},
            {0x5E, '^'},
            {0x5F, '_'},
            //
            {0x60, '`'},
            {0x61, 'a'},
            {0x62, 'b'},
            {0x63, 'c'},
            {0x64, 'd'},
            {0x65, 'e'},
            {0x66, 'f'},
            {0x67, 'g'},
            {0x68, 'h'},
            {0x69, 'i'},
            {0x6A, 'j'},
            {0x6B, 'k'},
            {0x6C, 'l'},
            {0x6D, 'm'},
            {0x6E, 'n'},
            {0x6F, 'o'},
            //
            {0x70, 'p'},
            {0x71, 'q'},
            {0x72, 'r'},
            {0x73, 's'},
            {0x74, 't'},
            {0x75, 'u'},
            {0x76, 'v'},
            {0x77, 'w'},
            {0x78, 'x'},
            {0x79, 'y'},
            {0x7A, 'z'},
            {0x7B, '{'},
            {0x7C, '|'},
            {0x7D, '}'},
            //{0x7E, '‾'},
            //{0x7F, '∇'}
        };

        std::string string_from_ia5(uint8_t *buf, int len)
        {
            std::string ret = "";
            for (int i = 0; i < len; i++)
            {
                uint8_t b = buf[i] & 0x7F;
                char c = ' ';
                if (table_xia5.count(b))
                    c = table_xia5[b];
                if (!isascii(c))
                    c = ' ';
                ret.push_back(c);
            }
            return ret;
        }

        std::string message_to_string(std::vector<uint8_t> buf, int presentation, bool egc)
        {
            std::string ret = "";

            if (presentation == 0)
            {
                for (int i = 0; i < (int)buf.size(); i++)
                {
                    uint8_t b = buf[i] & 0x7F;
                    char c = ' ';
                    if (table_xia5.count(b))
                        c = table_xia5[b];
                    ret.push_back(c);
                }
            }
            else if (presentation == 7)
            {
                for (int i = 0; i < (int)buf.size(); i++)
                {
                    char c = *((char *)&buf[i]);
                    ret.push_back(c);
                }
            }

            // Ensure it's all ASCII just in case
            for (char &c : ret)
                if (!isascii(c))
                    c = ' ';

            // Remove duplicate spaces
            // auto it = std::unique(ret.begin(), ret.end(), [](char const &lhs, char const &rhs)
            //                      { return lhs == rhs && iswspace(lhs); });
            // ret.erase(it, ret.end());

            if (ret.size() > 0 && !egc)
                ret.erase(ret.end() - 1, ret.end());

            return ret;
        }

        std::string get_service_code_and_address_name(int code)
        {
            switch (code)
            {
            case 0x00:
                return "System, All ships (general call)";
            case 0x02:
                return "FleetNET, Group Call";
            case 0x04:
                return "SafetyNET, Navigational, Meteorological or Piracy Warning to a Rectangular Area";
            case 0x11:
                return "System, Inmarsat System Message";
            case 0x13:
                return "SafetyNET, Navigational, Meteorological or Piracy Coastal Warning";
            case 0x14:
                return "SafetyNET, Shore-to-Ship Distress Alert to Circular Area";
            case 0x23:
                return "System, EGC System Message";
            case 0x24:
                return "SafetyNET, Navigational, Meteorological or Piracy Warning to a Circular Area";
            case 0x31:
                return "SafetyNET, NAVAREA/METAREA Warning, MET Forecast or Piracy Warning to NAVAREA/METAREA";
            case 0x33:
                return "System, Download Group Identity";
            case 0x34:
                return "SafetyNET, SAR Coordination to a Rectangular Area";
            case 0x44:
                return "SafetyNET, SAR Coordination to a Circular Area";
            case 0x72:
                return "FleetNET, Chart Correction Service";
            case 0x73:
                return "SafetyNET, Chart Correction Service for Fixed Areas";
            default:
                return "Unknown";
            }
        }

        std::string get_priority(int priority)
        {
            switch (priority)
            {
            case -1:
                return "Message";
            case 0:
                return "Routine";
            case 1:
                return "Safety";
            case 2:
                return "Urgency";
            case 3:
                return "Distress";
            default:
                return "Unknown";
            }
        }

        int get_address_length(int messageType)
        {
            switch (messageType)
            {
            case 0x00:
                return 3;
            case 0x11:
            case 0x31:
                return 4;
            case 0x02:
            case 0x72:
                return 5;
            case 0x13:
            case 0x23:
            case 0x33:
            case 0x73:
                return 6;
            case 0x04:
            case 0x14:
            case 0x24:
            case 0x34:
            case 0x44:
                return 7;
            default:
                return 3;
            }
        }

        double parse_uplink_freq_mhz(uint8_t *b)
        {
            return ((b[0] << 8 | (uint16_t)b[1]) - 6000) * 0.0025 + 1626.5;
        }

        double parse_downlink_freq_mhz(uint8_t *b)
        {
            return ((b[0] << 8 | (uint16_t)b[1]) - 8000) * 0.0025 + 1530.5;
        }

        std::string service4_name(uint8_t service_b)
        {
            if (service_b == 0)
                return "Store And Forward";
            else if (service_b == 1)
                return "Half Duplex Data";
            else if (service_b == 2)
                return "Circuit Switched Data (no ARQ)";
            else if (service_b == 3)
                return "Circuit Switched Data (ARQ)";
            else if (service_b == 0xE)
                return "Message Performance Verification";
            else
                return "Unknown";
        }

        std::string direction2_name(uint8_t direction_b)
        {
            if (direction_b == 0)
                return "To Mobile";
            else if (direction_b == 1)
                return "From Mobile";
            else if (direction_b == 3)
                return "Both";
            else
                return "Unknown";
        }

        int get_packet_frm_id(nlohmann::json pkt)
        {
            return pkt["descriptor"].get<pkts::PacketDescriptor>().type;
        }
    }
}