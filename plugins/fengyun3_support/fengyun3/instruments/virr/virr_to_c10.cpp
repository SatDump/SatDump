#include "virr_to_c10.h"
#include <filesystem>
#include "logger.h"
#include "../../fengyun3.h"

namespace fengyun3
{
    namespace virr
    {
        VIRRToC10::VIRRToC10()
        {
        }

        VIRRToC10::~VIRRToC10()
        {
        }

        void VIRRToC10::open(std::string path)
        {
            temp_path = path;
            output_c10.open(path, std::ios::binary);
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

        void VIRRToC10::close(time_t timestamp, int satellite)
        {
            output_c10.close();

            // Name the file properly
            std::string hpt_prefix = "FY3x_";

            if (satellite == FY3_A_SCID)
                hpt_prefix = "FY3A_";
            else if (satellite == FY3_B_SCID)
                hpt_prefix = "FY3B_";
            else if (satellite == FY3_C_SCID)
                hpt_prefix = "FY3C_";

            std::string hpt_filename = hpt_prefix + getHRPTReaderTimeStamp(timestamp) + ".C10";

            std::string path = std::filesystem::path(temp_path).parent_path().string() + "/" + hpt_filename;

            std::filesystem::rename(temp_path, path);
            logger->info("Saved .C10 file at " + path);
        }

        void VIRRToC10::work(std::vector<uint8_t> &pkt)
        {
            // Clean it up
            std::fill(c10_buffer, &c10_buffer[27728], 0);

            // Write header
            c10_buffer[0] = 0xa1;
            c10_buffer[1] = 0x16;
            c10_buffer[2] = 0xfd;
            c10_buffer[3] = 0x71;
            c10_buffer[4] = 0x9d;
            c10_buffer[5] = 0x83;
            c10_buffer[6] = 0xc9;
            c10_buffer[7] = 0x50;
            c10_buffer[8] = 0x34;
            c10_buffer[9] = 0x00;
            c10_buffer[10] = 0x3d;

            // Timestamp
            c10_buffer[11] = 0b00101000 | (((pkt[26044] & 0b111111) << 2 | pkt[26045] >> 6) & 0b111);
            c10_buffer[12] = (pkt[26045] & 0b111111) << 2 | pkt[26046] >> 6;
            c10_buffer[13] = (pkt[26046] & 0b111111) << 2 | pkt[26047] >> 6;
            c10_buffer[14] = (pkt[26047] & 0b111111) << 2 | pkt[26048] >> 6;

            // Imagery, shifted by 2 bits
            for (int i = 0; i < 25600 + 16; i++)
                c10_buffer[2000 + i] = (pkt[436 + i] & 0b111111) << 2 | pkt[437 + i] >> 6;

            // Last marker
            c10_buffer[27613] = 0b0000010;

            // Write it out
            output_c10.write((char *)c10_buffer, 27728);
        }

    } // namespace avhrr
} // namespace metop