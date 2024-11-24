#pragma once

#include <cstdint>
#include "nlohmann/json.hpp"

// While the full frame layout is known for BIS-M, how to interpret
// the data is not. Uncomment the define below to dump out all BIS-M
// data as uints. Otherwise, only times are extracted

//#define BISM_FULL_DUMP 1

namespace meteor
{
    namespace bism
    {
        struct Type0
        {
            time_t clock_time_moscow;

#ifdef BISM_FULL_DUMP
            uint32_t clock_mode;
            uint64_t coord_x;
            uint64_t coord_y;
            uint64_t coord_z;
            uint32_t velocity_x;
            uint32_t velocity_y;
            uint32_t velocity_z;

            uint64_t angular_velocity_aquisition_time;
            uint32_t angular_velocity_x1;
            uint32_t angular_velocity_y1;
            uint32_t angular_velocity_y2;
            uint32_t angular_velocity_z2;
            uint32_t angular_velocity_z3;
            uint32_t angular_velocity_x3;
            uint8_t  angular_velocity_code_word;
#endif
        };

        struct Type255
        {
            uint32_t clock_time_moscow;
            uint16_t STAT_status_word;
            uint32_t bokzm_measurement_time_moscow;

#ifdef BISM_FULL_DUMP
            uint32_t clock_status_word_gen_time_moscow;
            uint16_t clock_status_word;

            uint64_t pitch_roll_aquisition_time;
            uint16_t pitch_angle_1;
            uint16_t roll_angle_1;
            uint16_t pitch_angle_2;
            uint16_t roll_angle_2;

            uint32_t additional_timestamp_delay_to_clock_moscow;

            uint64_t angular_velocity_aquisition_time;
            uint32_t angular_velocity_x1;
            uint32_t angular_velocity_y1;
            uint32_t angular_velocity_y2;
            uint32_t angular_velocity_z2;
            uint32_t angular_velocity_z3;
            uint32_t angular_velocity_x3;
            uint8_t  angular_velocity_code_word;
#endif
        };

#ifdef BISM_FULL_DUMP
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Type0, clock_time_moscow, clock_mode, coord_x, coord_y, coord_z, angular_velocity_aquisition_time,
            angular_velocity_x1, angular_velocity_y1, angular_velocity_y2, angular_velocity_z2, angular_velocity_z3, angular_velocity_x3,
            angular_velocity_code_word);
        NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Type255, clock_time_moscow, STAT_status_word, bokzm_measurement_time_moscow, clock_status_word_gen_time_moscow,
            clock_status_word, pitch_roll_aquisition_time, pitch_angle_1, roll_angle_1, pitch_angle_2, roll_angle_2, additional_timestamp_delay_to_clock_moscow,
            angular_velocity_aquisition_time, angular_velocity_x1, angular_velocity_y1, angular_velocity_y2, angular_velocity_z2, angular_velocity_z3,
            angular_velocity_x3, angular_velocity_code_word);
#endif

        class BISMReader
        {
        private:
            time_t timestamp_offset;
            std::vector<Type0> type0_frames;
            std::vector<Type255> type255_frames;

        public:
            BISMReader(int year_ov);
            ~BISMReader();

            void work(uint8_t *data);
            time_t get_last_day_moscow();
            int get_lines();
            void save(std::string directory);
        };
    }
}