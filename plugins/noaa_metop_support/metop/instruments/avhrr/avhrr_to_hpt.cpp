#include "avhrr_to_hpt.h"
#include <filesystem>
#include "logger.h"
#include "../../metop.h"

namespace metop
{
    namespace avhrr
    {
        AVHRRToHpt::AVHRRToHpt()
        {
        }

        AVHRRToHpt::~AVHRRToHpt()
        {
        }

        void AVHRRToHpt::open(std::string path)
        {
            temp_path = path;
            output_hpt.open(path, std::ios::binary);
        }

        std::string getHRPTReaderTimeStamp(const time_t timevalue)
        {
            std::tm *timeReadable = gmtime(&timevalue);

            std::string timestamp = std::to_string(timeReadable->tm_year + 1900) + "-" +                                                                               // Year yyyy
                                    (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" + // Month MM
                                    (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "-" +          // Day dd
                                    (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) +                // Hour HH
                                    (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min));                    // Minutes mm

            return timestamp;
        }

        void AVHRRToHpt::close(time_t timestamp, int satellite)
        {
            output_hpt.close();

            // Name the file properly
            std::string hpt_prefix = "M0x_";

            if (satellite == METOP_A_SCID)
                hpt_prefix = "M02_";
            else if (satellite == METOP_B_SCID)
                hpt_prefix = "M03_";
            else if (satellite == METOP_C_SCID)
                hpt_prefix = "M04_";

            std::string hpt_filename = hpt_prefix + getHRPTReaderTimeStamp(timestamp) + ".hpt";

            std::string path = std::filesystem::path(temp_path).parent_path().string() + "/" + hpt_filename;

            std::filesystem::rename(temp_path, path);
            logger->info("Saved .hpt file at " + path);
        }

        void AVHRRToHpt::work(ccsds::CCSDSPacket &pkt)
        {
            if (pkt.payload.size() < 12960)
                return;

            // Clean it up
            std::fill(hpt_buffer, &hpt_buffer[13864], 0);

            // Write header
            hpt_buffer[0] = 0xa1;
            hpt_buffer[1] = 0x16;
            hpt_buffer[2] = 0xfd;
            hpt_buffer[3] = 0x71;
            hpt_buffer[4] = 0x9d;
            hpt_buffer[5] = 0x83;
            hpt_buffer[6] = 0xc9;

            // Counter
            hpt_buffer[7] = 0b01010 << 3 | (counter & 0b111) << 1 | 0b1;
            counter++;
            if (counter == 4)
                counter = 0;

            // Timestamp
            uint16_t days = pkt.payload[0] << 8 | pkt.payload[1];
            days -= 502;         // Scale from 1/1/2000 to days since first frame
            days &= 0b111111111; // Cap to 9-bits

            hpt_buffer[10] = days >> 1;
            hpt_buffer[11] = (days & 0b1) << 7 | 0b0101 << 3 | (pkt.payload[2] & 0b111);
            std::memcpy(&hpt_buffer[12], &pkt.payload[3], 3);

            // Other marker
            hpt_buffer[21] = counter2 == 0 ? 0 : 0b1100;
            hpt_buffer[22] = counter2 == 0 ? 0 : 0b0011;
            hpt_buffer[24] = counter2 == 0 ? 0 : 0b11000000;
            counter2++;
            if (counter2 == 5)
                counter2 = 0;

            // Imagery
            std::memcpy(&hpt_buffer[937], &pkt.payload[76], 12800);

            // Write it out
            output_hpt.write((char *)hpt_buffer, 13864);
        }

    } // namespace avhrr
} // namespace metop