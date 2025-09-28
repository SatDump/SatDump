#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"
#include "nlohmann/json.hpp"
#include <cstdint>

namespace metopsg
{
    namespace metimage
    {
        class METImageReader
        {
        private:
            const int SUBPKTN = 56; // Number of 25-pixels "groups" in each packet
            const int PKTSEGN = 58; // Number of packets per segment
            const int LWIDTH = SUBPKTN * PKTSEGN;

            struct ch_cfg_t
            {
                int index;
                int depth;
                bool rev;
            };

            std::map<int, ch_cfg_t> ch_cfg = {
                {30, {0, 18, true}},   //
                {28, {1, 18, true}},   //
                {26, {2, 18, true}},   //
                {25, {3, 17, true}},   //
                {23, {4, 17, true}},   //
                {24, {5, 17, true}},   //
                {22, {6, 17, true}},   //
                {4, {7, 17, false}},   //
                {12, {8, 17, false}},  //
                {8, {9, 17, false}},   //
                {17, {10, 17, false}}, //
                {15, {11, 18, false}}, //
                {16, {12, 18, false}}, //
                {20, {13, 18, false}}, //
                {40, {14, 18, false}}, //
                {39, {15, 18, false}}, //
                {37, {16, 17, false}}, //
                {34, {17, 17, false}}, //
                {35, {18, 17, false}}, //
                {33, {19, 17, false}}, //
            };

        public:
            const int out_ch_n[20] = {30, 28, 26, 25, 23, 24, 22, 4, 12, 8, 17, 15, 16, 20, 40, 39, 37, 34, 35, 33};

        public:
            int segments;
            std::vector<uint16_t> wip_lines[20][24];
            std::vector<uint16_t> channels[20];
            std::vector<double> timestamps;

        public:
            METImageReader();
            ~METImageReader();

            void work(ccsds::CCSDSPacket &packet);

            image::Image getChannel(int c);
        };
    } // namespace metimage
} // namespace metopsg