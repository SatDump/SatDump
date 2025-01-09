#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace meteor
{
    namespace msumr
    {
        namespace lrpt
        {
            class Segment
            {
            private:
                bool meteorm2x_mode;

            public:
                uint16_t day_time;
                uint32_t ms_time;
                uint16_t us_time;
                double timestamp;

                uint8_t MCUN;
                uint8_t QT;
                uint8_t DC;
                uint8_t AC;
                uint16_t QFM;
                uint8_t QF;
                bool valid;
                bool partial;
                uint8_t lines[8][14 * 8] = {{0}};

                bool isValid();

                Segment();
                Segment(uint8_t *data, int length, bool partial, bool meteorm2x_mode);
                ~Segment();

                void decode(uint8_t *data, int length);
            };
        } // namespace lrpt
    }     // namespace msumr
} // namespace meteor