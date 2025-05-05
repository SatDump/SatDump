#include <fstream>
#include "bism_reader.h"
#include "common/utils.h"

namespace meteor
{
    namespace bism
    {
        BISMReader::BISMReader(int year_ov)
        {
            time_t curr_time = time(NULL);
            struct tm timeinfo_struct;
#ifdef _WIN32
            memcpy(&timeinfo_struct, gmtime(&curr_time), sizeof(struct tm));
#else
            gmtime_r(&curr_time, &timeinfo_struct);
#endif

            if (year_ov != -1)
                timeinfo_struct.tm_year = year_ov - 1900;

            // Reset to be December 31 before the last leap year,
            // Ignorning the 100-year thing since Meteor probably does too
            // If this code is still used in 2100, we need help.
            timeinfo_struct.tm_year -= (timeinfo_struct.tm_year % 4) + 1;
            timeinfo_struct.tm_mday = 31;
            timeinfo_struct.tm_mon = 11;
            timeinfo_struct.tm_sec = 0;
            timeinfo_struct.tm_min = 0;
            timeinfo_struct.tm_hour = 0;

            timestamp_offset = timegm(&timeinfo_struct) - 1;
        }

        BISMReader::~BISMReader()
        {
        }

        void BISMReader::work(uint8_t *data)
        {
            if (data[4] == 0)
            {
                Type0 wip_frame;
                wip_frame.clock_time_moscow = timestamp_offset + ((uint32_t)data[9] << 24 | (uint32_t)data[8] << 16 | (uint32_t)data[7] << 8 | (uint32_t)data[6]);

#ifdef BISM_FULL_DUMP
                wip_frame.clock_mode = (uint32_t)data[13] << 24 | (uint32_t)data[12] << 16 | (uint32_t)data[11] << 8 | (uint32_t)data[10];

                wip_frame.coord_x = (uint64_t)data[20] << 48 | (uint64_t)data[19] << 40 | (uint64_t)data[18] << 32 |
                    (uint64_t)data[17] << 24 | (uint64_t)data[16] << 16 | (uint64_t)data[15] << 8 | (uint64_t)data[14];
                wip_frame.coord_y = (uint64_t)data[28] << 48 | (uint64_t)data[27] << 40 | (uint64_t)data[26] << 32 |
                    (uint64_t)data[25] << 24 | (uint64_t)data[24] << 16 | (uint64_t)data[23] << 8 | (uint64_t)data[22];
                wip_frame.coord_z = (uint64_t)data[36] << 48 | (uint64_t)data[35] << 40 | (uint64_t)data[34] << 32 |
                    (uint64_t)data[33] << 24 | (uint64_t)data[32] << 16 | (uint64_t)data[31] << 8 | (uint64_t)data[30];

                wip_frame.velocity_x = (uint32_t)data[41] << 24 | (uint32_t)data[40] << 16 | (uint32_t)data[39] << 8 | (uint32_t)data[38];
                wip_frame.velocity_y = (uint32_t)data[45] << 24 | (uint32_t)data[44] << 16 | (uint32_t)data[43] << 8 | (uint32_t)data[42];
                wip_frame.velocity_z = (uint32_t)data[49] << 24 | (uint32_t)data[48] << 16 | (uint32_t)data[47] << 8 | (uint32_t)data[46];

                // Spare words 50-55; ignoring

                wip_frame.angular_velocity_aquisition_time = (uint64_t)data[61] << 40 | (uint64_t)data[60] << 32 |
                    (uint64_t)data[59] << 24 | (uint64_t)data[58] << 16 | (uint64_t)data[57] << 8 | (uint64_t)data[56];

                wip_frame.angular_velocity_x1 = (uint32_t)data[65] << 24 | (uint32_t)data[64] << 16 | (uint32_t)data[63] << 8 | (uint32_t)data[62];
                wip_frame.angular_velocity_y1 = (uint32_t)data[69] << 24 | (uint32_t)data[68] << 16 | (uint32_t)data[67] << 8 | (uint32_t)data[66];
                wip_frame.angular_velocity_y2 = (uint32_t)data[73] << 24 | (uint32_t)data[72] << 16 | (uint32_t)data[71] << 8 | (uint32_t)data[70];
                wip_frame.angular_velocity_z2 = (uint32_t)data[77] << 24 | (uint32_t)data[76] << 16 | (uint32_t)data[75] << 8 | (uint32_t)data[74];
                wip_frame.angular_velocity_z3 = (uint32_t)data[81] << 24 | (uint32_t)data[80] << 16 | (uint32_t)data[79] << 8 | (uint32_t)data[78];
                wip_frame.angular_velocity_x3 = (uint32_t)data[85] << 24 | (uint32_t)data[84] << 16 | (uint32_t)data[83] << 8 | (uint32_t)data[82];

                wip_frame.angular_velocity_code_word = data[86];

                // One spare word at 87; ignoring
#endif
                type0_frames.emplace_back(wip_frame);
            }

            else if (data[4] == 255)
            {
                Type255 wip_frame;
                wip_frame.clock_time_moscow = timestamp_offset + ((uint32_t)data[9] << 24 | (uint32_t)data[8] << 16 | (uint32_t)data[7] << 8 | (uint32_t)data[6]);
                wip_frame.STAT_status_word = (uint16_t)data[11] << 8 | (uint16_t)data[10];
                wip_frame.bokzm_measurement_time_moscow = timestamp_offset + ((uint32_t)data[15] << 24 | (uint32_t)data[14] << 16 | (uint32_t)data[13] << 8 | (uint32_t)data[12]);

#ifdef BISM_FULL_DUMP
                // Location of body - fixed coordinate system relative to default coordinate system. Bytes 16-31.
                // Skipping for now

                wip_frame.clock_status_word_gen_time_moscow = timestamp_offset +
                    ((uint32_t)data[35] << 24 | (uint32_t)data[34] << 16 | (uint32_t)data[33] << 8 | (uint32_t)data[32]);
                wip_frame.clock_status_word = (uint16_t)data[37] << 8 | (uint16_t)data[36];
                wip_frame.pitch_roll_aquisition_time = (uint64_t)data[43] << 40 | (uint64_t)data[42] << 32 | (uint64_t)data[41] << 24 | (uint64_t)data[40] << 16 |
                    (uint64_t)data[39] << 8 | (uint64_t)data[38];

                wip_frame.pitch_angle_1 = (uint16_t)data[45] << 8 | (uint16_t)data[44];
                wip_frame.roll_angle_1 = (uint16_t)data[47] << 8 | (uint16_t)data[46];
                wip_frame.pitch_angle_2 = (uint16_t)data[49] << 8 | (uint16_t)data[48];
                wip_frame.roll_angle_2 = (uint16_t)data[51] << 8 | (uint16_t)data[50];

                wip_frame.additional_timestamp_delay_to_clock_moscow = timestamp_offset +
                    ((uint32_t)data[55] << 24 | (uint32_t)data[54] << 16 | (uint32_t)data[53] << 8 | (uint32_t)data[52]);
                wip_frame.angular_velocity_aquisition_time = (uint64_t)data[61] << 40 | (uint64_t)data[60] << 32 |
                    (uint64_t)data[59] << 24 | (uint64_t)data[58] << 16 | (uint64_t)data[57] << 8 | (uint64_t)data[56];

                wip_frame.angular_velocity_x1 = (uint32_t)data[65] << 24 | (uint32_t)data[64] << 16 | (uint32_t)data[63] << 8 | (uint32_t)data[62];
                wip_frame.angular_velocity_y1 = (uint32_t)data[69] << 24 | (uint32_t)data[68] << 16 | (uint32_t)data[67] << 8 | (uint32_t)data[66];
                wip_frame.angular_velocity_y2 = (uint32_t)data[73] << 24 | (uint32_t)data[72] << 16 | (uint32_t)data[71] << 8 | (uint32_t)data[70];
                wip_frame.angular_velocity_z2 = (uint32_t)data[77] << 24 | (uint32_t)data[76] << 16 | (uint32_t)data[75] << 8 | (uint32_t)data[74];
                wip_frame.angular_velocity_z3 = (uint32_t)data[81] << 24 | (uint32_t)data[80] << 16 | (uint32_t)data[79] << 8 | (uint32_t)data[78];
                wip_frame.angular_velocity_x3 = (uint32_t)data[85] << 24 | (uint32_t)data[84] << 16 | (uint32_t)data[83] << 8 | (uint32_t)data[82];

                wip_frame.angular_velocity_code_word = data[86];

                // One spare word at 87; ignoring
#endif
                type255_frames.emplace_back(wip_frame);
            }
        }

        time_t BISMReader::get_last_day_moscow()
        {
            time_t full_time;
            if (!type0_frames.empty())
                full_time = type0_frames.back().clock_time_moscow;
            else if (!type255_frames.empty())
                full_time = type255_frames.back().clock_time_moscow;
            else
                return 0;

            struct tm timeinfo_struct;

#ifdef _WIN32
            memcpy(&timeinfo_struct, gmtime(&full_time), sizeof(struct tm));
#else
            gmtime_r(&full_time, &timeinfo_struct);
#endif

            timeinfo_struct.tm_hour = 0;
            timeinfo_struct.tm_min = 0;
            timeinfo_struct.tm_sec = 0;

            return timegm(&timeinfo_struct);
        }

        int BISMReader::get_lines()
        {
            return type0_frames.size() + type255_frames.size();
        }

        void BISMReader::save(std::string directory)
        {
#ifdef BISM_FULL_DUMP
            std::ofstream write_type0(directory + "/Type0.json");
            write_type0 << ((nlohmann::json)type0_frames).dump(4);
            write_type0.close();

            std::ofstream write_type255(directory + "/Type255.json");
            write_type255 << ((nlohmann::json)type255_frames).dump(4);
            write_type255.close();
#endif
        }
    }
}