#include "pkt_structs.h"

namespace inmarsat
{
    namespace aero
    {
        std::string pkt_type_to_name(uint8_t id)
        {
            // BROADCAST
            if (id == 0x00)
                return "Reserved 0x00";
            else if (id == 0x01)
                return "Fill-in Signal Unit";

            else if (id == 0x02)
                return "AES System Table Broadcast (GES Psmc and Rsmc channels PARTIAL)";
            else if (id == 0x03)
                return "AES System Table Broadcast (Beam Identification PARTIAL)";
            else if (id == 0x04)
                return "AES System Table Broadcast (GES Beam Support PARTIAL)";

            else if (id == 0x05)
                return "AES System Table Broadcast (GES Psmc and Rsmc channels COMPLETE)";
            else if (id == 0x06)
                return "AES System Table Broadcast (Beam Identification COMPLETE)";
            else if (id == 0x07)
                return "AES System Table Broadcast (GES Beam Support COMPLETE)";

            else if (id == 0x08)
                return "System Broadcast Selective Release";
            else if (id == 0x09)
                return "System Broadcast Universal Time";

            else if (id == 0x0A)
                return "AES System Table Broadcast (Index)";
            else if (id == 0x0B)
                return "AES System Table Broadcast (Satellite Identification PARTIAL)";
            else if (id == 0x0C)
                return "AES System Table Broadcast (Satellite Identification COMPLETE)";
            else if (id == 0x0D)
                return "AES System Table Broadcast (2nd Series Of GES Psmc and Rsmc channels COMPLETE)";

            else if (id == 0x0E)
                return "Reserved 0x0E";

            // SYSTEM LOG-ON/LOG-OFF
            else if (id == 0x10)
                return "Log-On Request";
            else if (id == 0x11)
                return "Log-On Confirm";
            else if (id == 0x12)
                return "Log Control (P Channel) Log-Off Request";
            else if (id == 0x13)
                return "Log Control (P Channel) Log-On Reject";
            else if (id == 0x14)
                return "Log Control (P Channel) Log-On Interrogation";
            else if (id == 0x15)
                return "Log-On Log-Off Acknowledge (P Channel)";
            else if (id == 0x16)
                return "Log Control (P Channel) Log-On Prompt";
            else if (id == 0x17)
                return "Log Control (P Channel) Data Channel Reassignment";

            else if (id == 0x18)
                return "Reserved 0x18";
            else if (id == 0x19)
                return "Reserved 0x19";

            // CALL INITIATION
            else if (id == 0x20)
                return "General Access Request Telephone / Call Annoucement";
            else if (id == 0x21)
                return "Call Information Service Address";
            else if (id == 0x22)
                return "Acess Request Data (R/T Channel)";
            else if (id == 0x23)
                return "Abreviated Access Request Telephone";

            else if (id == 0x24)
                return "Reserved 0x24";
            else if (id == 0x25)
                return "Reserved 0x25";
            else if (id == 0x26)
                return "Reserved 0x26";
            else if (id == 0x27)
                return "Reserved 0x27";

            else if (id == 0x28)
                return "Data EIRP Table Broadcast COMPLETE";
            else if (id == 0x29)
                return "Data EIRP Table Broadcast PARTIAL";

            // CALL PROGRESS
            else if (id == 0x30)
                return "Call Progress";
            else if (id == 0x31)
                return "C Channel Assignment Distress";
            else if (id == 0x32)
                return "C Channel Assignment Flight Safety";
            else if (id == 0x33)
                return "C Channel Assignment Other Safety";
            else if (id == 0x34)
                return "C Channel Assignment Non Safety";

            else if (id == 0x35)
                return "Reserved 0x35";
            else if (id == 0x36)
                return "Reserved 0x36";
            else if (id == 0x37)
                return "Reserved 0x37";
            else if (id == 0x38)
                return "Reserved 0x38";
            else if (id == 0x39)
                return "Reserved 0x39";

            // CHANNEL INFORMATION
            else if (id == 0x40)
                return "P/R Channel Control (ISU)";
            else if (id == 0x41)
                return "T Channel Control (ISU)";

            // TDMA RESERVATION
            else if (id == 0x50)
                return "Unsolicited Reservation";
            else if (id == 0x51)
                return "T Channel Assignement";
            else if (id == 0x52)
                return "Reserved 0x52";
            else if (id == 0x53)
                return "Reservation Forthcoming (RFC)";

            // ACKNOWLEDGEMENT
            else if (id == 0x60)
                return "Telephony Acknowledge (P/C or R Channel)";
            else if (id == 0x61)
                return "Request For Acknowledgement (RQA) (P Channel)";
            else if (id == 0x62)
                return "Acknowledge (RACK / TACK P Channel, PACK R Channel)";

            else if (id == 0x63)
                return "Reserved 0x63";
            else if (id == 0x64)
                return "Reserved 0x64";
            else if (id == 0x65)
                return "Reserved 0x65";

            // USER DATA
            else if (id == 0x70)
                return "Reserved 0x70";

            else if (id == 0x71)
                return "User Data (ISU) RLS (P/T Channel)";
            else if (id == 0x72)
                return "Retransmission Header (RTX) (P/T Channel)";

            else if (id == 0x73)
                return "Reserved 0x73";

            else if (id == 0x74)
                return "User Data (3 Octet LSDU) RLS (P/T Channel)";

            else if (id == 0x75)
                return "Reserved 0x75";

            else if (id == 0x76)
                return "User Data (4 Octet LSDU) RLS (P/T Channel)";

            else if (id == 0x80)
                return "Broadcast Reserved";
            else if (id == 0x81)
                return "AES System Table Broadcat Spot Beam Series GES P/R Channel PARTIAL";
            else if (id == 0x82)
                return "AES System Table Broadcat Spot Beam Series GES Beam Support PARTIAL";
            else if (id == 0x83)
                return "AES System Table Broadcat Spot Beam Series GES P/R Channel COMPLETE";
            else if (id == 0x84)
                return "AES System Table Broadcat Spot Beam Series GES Beam Support COMPLETE";
            else if (id == 0x85)
                return "AES System Table Broadcat Spot Beam Series Index";
            else if (id == 0x86)
                return "AES System Table Broadcat Spot Beam Series Satellite/Beam ID PARTIAL";
            else if (id == 0x87)
                return "AES System Table Broadcat Spot Beam Series Satellite/Beam ID COMPLETE";
            else if (id == 0x88)
                return "AES System Table Broadcast (2nd Series Of GES P/R channel COMPLETE)";

            else if (id == 0x89)
                return "Reserved 0x89";

            // SUBSEQUENT SIGNAL UNITS
            else if ((id & 0xC0) == 0xC0)
                return "SSU";

            else
                return "Unknown";
        }

        uint16_t compute_crc(uint8_t *buf, int len)
        {
            uint16_t crc = 0xFFFF;
            uint8_t b, b1, b2;
            for (int i = 0; i < len; i++)
            {
                b = buf[i];
                for (int j = 0; j < 8; j++)
                {
                    b1 = b & 1;
                    b2 = crc & 1;
                    b >>= 1;
                    crc >>= 1;
                    if (b2 ^ b1)
                        crc = crc ^ 0x8408;
                }
            }
            return crc ^ 0xFFFF;
        }

        bool check_crc(uint8_t *pkt)
        {
            uint16_t crc_calc = compute_crc(pkt, 10);
            uint16_t crc_pack = pkt[11] << 8 | pkt[10];
            return crc_calc == crc_pack;
        }
    }
}