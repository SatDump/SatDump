#pragma once

#include <vector>
#include "image/image.h"

namespace fengyun3
{
    namespace mersi
    {
        class MERSIReader
        {
        public:
            // MERSI Configuration
            int ch_cnt_250;
            int ch_cnt_1000;
            int ch250_width;

            int frame_head_size;
            int frame_scan_250_size;
            int frame_scan_1000_size;

            int imagery_offset_bytes;
            int imagery_offset_bits;

            int calib_byte_offset;
            int calib_length;

            // Computed stuff
            int counter_250_end;
            int counter_max;
            int ch1000_width;

            // Timestamp stuff
            double ms_scale = 1e3;
            int timestamp3g_mode = false;

        protected:
            void init()
            {
                // Initialize from settings
                channels_250m.resize(ch_cnt_250);
                channels_1000m.resize(ch_cnt_1000);

                repacked_mersi_line = new uint16_t[ch250_width + 10];
                mersi_line_buffer = new uint8_t[frame_scan_250_size / 8];

                counter_250_end = ch_cnt_250 * 40;
                counter_max = ch_cnt_250 * 40 + ch_cnt_1000 * 10;
                ch1000_width = ch250_width / 4;

                repacked_calib = new uint16_t[calib_length * 2];

                for (int i = 0; i < ch_cnt_250; i++)
                    channels_250m[i].resize(ch250_width * 40 * 2);
                for (int i = 0; i < ch_cnt_1000; i++)
                    channels_1000m[i].resize(ch1000_width * 10 * 2);
                calibration.resize(calib_length * 3);
                segments = 0;
            }

        private:
            std::vector<uint16_t> calibration;
            uint16_t *repacked_calib;

            std::vector<std::vector<uint16_t>> channels_250m;
            std::vector<std::vector<uint16_t>> channels_1000m;

            uint16_t *repacked_mersi_line;
            uint8_t *mersi_line_buffer;

            // Deframer part
            uint64_t bit_shifter_head = 0;
            uint64_t bit_shifter_scan = 0;
            std::vector<uint8_t> current_frame;
            bool was_head = false;
            bool in_frame = false;
            int bits_wrote_output_frame;
            uint8_t defra_byte_shifter;

            int current_frame_size = -1;

            // Timestamp
            double last_timestamp;

            void process_curr();
            void process_head();
            void process_scan();

        public:
            int segments;
            std::vector<double> timestamps;

        public:
            MERSIReader();
            ~MERSIReader();
            void work(uint8_t *data, int size);
            image::Image getChannel(int channel);
        };

        class MERSI1Reader : public MERSIReader
        {
        public:
            MERSI1Reader()
            {
                // Configuration for MERSI-1
                ch_cnt_250 = 5;
                ch_cnt_1000 = 15;
                ch250_width = 8192;

                frame_head_size = 206048;
                frame_scan_250_size = 99120;
                frame_scan_1000_size = 25392;

                imagery_offset_bytes = 92;
                imagery_offset_bits = 6;

                calib_byte_offset = 92;
                calib_length = 17102;

                MERSIReader::init();
            }
        };

        class MERSI2Reader : public MERSIReader
        {
        public:
            MERSI2Reader()
            {
                // Configuration for MERSI-2
                ch_cnt_250 = 6;
                ch_cnt_1000 = 19;
                ch250_width = 8192;

                frame_head_size = 1329256;
                frame_scan_250_size = 98856;
                frame_scan_1000_size = 25128;

                imagery_offset_bytes = 59;
                imagery_offset_bits = 6;

                calib_byte_offset = 551;
                calib_length = 110400;

                MERSIReader::init();
            }
        };

        class MERSI3Reader : public MERSIReader
        {
        public:
            MERSI3Reader()
            {
                // Configuration for MERSI-3
                ch_cnt_250 = 6;
                ch_cnt_1000 = 19;
                ch250_width = 8192;

                frame_head_size = 1329256;
                frame_scan_250_size = 98904;
                frame_scan_1000_size = 25176;

                imagery_offset_bytes = 65;
                imagery_offset_bits = 6;

                calib_byte_offset = 551;
                calib_length = 13292256;

                ms_scale = 1e4;
                // timestamp3g_mode = true;

                MERSIReader::init();
            }
        };

        class MERSILLReader : public MERSIReader
        {
        public:
            MERSILLReader()
            {
                // Configuration for MERSI-LL
                ch_cnt_250 = 2;
                ch_cnt_1000 = 16;
                ch250_width = 6144;

                frame_head_size = 419176;
                frame_scan_250_size = 74976;
                frame_scan_1000_size = 19680;

                imagery_offset_bytes = 146;
                imagery_offset_bits = 6;

                calib_byte_offset = 551;
                calib_length = 34560;

                ms_scale = 1e4;

                MERSIReader::init();
            }
        };

        class MERSIRMReader : public MERSIReader
        {
        public:
            MERSIRMReader()
            {
                // Configuration for MERSI-RM
                ch_cnt_250 = 0;
                ch_cnt_1000 = 8;
                ch250_width = 1560 * 4;

                frame_head_size = 42864;
                frame_scan_250_size = 18816;
                frame_scan_1000_size = 18816;

                imagery_offset_bytes = 2;
                imagery_offset_bits = 6;

                calib_byte_offset = 0;
                calib_length = 5358;

                ms_scale = 1e4;
                timestamp3g_mode = true;

                MERSIReader::init();
            }
        };
    } // namespace avhrr
} // namespace fengyun3