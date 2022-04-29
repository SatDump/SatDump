#include "mersi_reader.h"
#include "common/repack.h"

namespace fengyun3
{
    namespace mersi
    {
        MERSIReader::MERSIReader()
        {
        }

        MERSIReader::~MERSIReader()
        {
            for (int i = 0; i < ch_cnt_250; i++)
                channels_250m[i].clear();
            for (int i = 0; i < ch_cnt_1000; i++)
                channels_1000m[i].clear();

            delete[] repacked_mersi_line;
            delete[] mersi_line_buffer;
        }

        void MERSIReader::process_head()
        {
#if 0
            logger->info("Head {:d} {:d}", current_frame.size() * 8, bits_wrote_output_frame);

            if (bits_wrote_output_frame != 1329256)
                return;

            int out = repackBytesTo12bits(&current_frame[551], current_frame.size() - 557, repacked_calib);


            for (int channel = 0; channel < 6; channel++)
            {
                double calib_tmp[40];

                for (int detector = 0; detector < 40; detector++)
                {
                    std::vector<double> avg;
                    for (int value = 0; value < 64; value++)
                        avg.push_back(repacked_calib[92000 + (channel * 40 + detector) * 64 + value] << 4);
                    calib_tmp[detector] = avg_overflowless(avg);
                }

                double init = calib_tmp[0];

                for (int detector = 0; detector < 40; detector++)
                {
                    double delta = calib_tmp[detector] - init;
                    if (delta > 2000)
                        return;
                    calib_channel_250[channel][detector] = delta;

                    logger->info("Avg is {:0.1f} for det {:d}", calib_channel_250[channel][detector], detector);
                }
            }

            for (int i = 0; i < 110400; i++)
                calibration[calib_line * 110400 + i] = repacked_calib[i] << 4;
            calib_line++;
            calibration.resize(110400 * (calib_line + 2));
#endif
        }

        void MERSIReader::process_scan()
        {
            int marker = (current_frame[0]) << 2 | current_frame[1] >> 6;

            if (marker == 0)
                segments++;

            current_frame.push_back(0);
            shift_array_left(&current_frame[imagery_offset_bytes], current_frame.size() - (imagery_offset_bytes + 1), imagery_offset_bits, current_frame.data());

            if (marker < counter_250_end) // 250m/px channels
            {
                int channel = marker / 40;
                int line = marker % 40;

                repackBytesTo12bits(current_frame.data(), (ch250_width * 12) / 8, repacked_mersi_line);

                for (int i = 0; i < ch250_width; i++)
                    channels_250m[channel][(segments * 40 + line) * ch250_width + i] = repacked_mersi_line[i] << 4;
            }
            else if (marker < counter_max) // 1000m/px channels
            {
                marker -= counter_250_end;
                int channel = marker / 10;
                int line = marker % 10;

                repackBytesTo12bits(current_frame.data(), (ch1000_width * 12) / 8, repacked_mersi_line);

                for (int i = 0; i < ch1000_width; i++)
                    channels_1000m[channel][(segments * 10 + line) * ch1000_width + i] = repacked_mersi_line[i] << 4;
            }

            for (int i = 0; i < ch_cnt_250; i++)
                channels_250m[i].resize(ch250_width * (segments + 2) * 40);
            for (int i = 0; i < ch_cnt_1000; i++)
                channels_1000m[i].resize(ch1000_width * (segments + 2) * 10);
        }

        void MERSIReader::process_curr()
        {
            // Fill up what we're missing
            for (int b = bits_wrote_output_frame; b < current_frame_size; b += 8)
                current_frame.push_back(0);

            if (was_head)
                process_head();
            else
                process_scan();
        }

        void MERSIReader::work(uint8_t *data, int size)
        {
            // Loop in all bytes
            for (int byte_n = 0; byte_n < size; byte_n++)
            {
                // Loop in all bits!
                for (int i = 7; i >= 0; i--)
                {
                    uint8_t bit = (data[byte_n] >> i) & 1;

                    bit_shifter_head = (bit_shifter_head << 1 | bit) % 281474976710656; // 2 ^ 48
                    bit_shifter_scan = (bit_shifter_scan << 1 | bit) % 268435456;       // 2 ^ 28

                    if (in_frame)
                    {
                        defra_byte_shifter = defra_byte_shifter << 1 | bit;
                        bits_wrote_output_frame++;

                        if (bits_wrote_output_frame % 8 == 0)
                            current_frame.push_back(defra_byte_shifter);

                        if (bits_wrote_output_frame == current_frame_size)
                        {
                            process_curr();
                            current_frame.clear();
                            in_frame = false;
                            current_frame_size = -1;
                        }

                        if (bits_wrote_output_frame == 80 && !was_head)
                        {
                            int marker = (current_frame[0]) << 2 | current_frame[1] >> 6;
                            if (marker < counter_250_end)
                                current_frame_size = frame_scan_250_size;
                            else
                                current_frame_size = frame_scan_1000_size;
                        }
                    }

                    if (bit_shifter_head == 0x55aa55aa55aa && !was_head)
                    {
                        if (current_frame.size() > 0)
                            process_curr();

                        bits_wrote_output_frame = 0;
                        current_frame_size = frame_head_size;
                        in_frame = true;
                        was_head = true;
                        current_frame.clear();
                    }
                    else if (bit_shifter_scan == 0b0111111111111000000000000100)
                    {
                        if (current_frame.size() > 0)
                            process_curr();

                        bits_wrote_output_frame = 0;
                        current_frame_size = frame_scan_250_size;
                        in_frame = true;
                        was_head = false;
                        current_frame.clear();
                    }
                }
            }
        }

        image::Image<uint16_t> MERSIReader::getChannel(int channel)
        {
            // if (channel == -1)
            //    return image::Image<uint16_t>(calibration.data(), 110400, calib_line, 1);

            if (channel < ch_cnt_250)
                return image::Image<uint16_t>(channels_250m[channel].data(), ch250_width, (segments + 1) * 40, 1);
            else
                return image::Image<uint16_t>(channels_1000m[channel - ch_cnt_250].data(), ch1000_width, (segments + 1) * 10, 1);
        }
    } // namespace avhrr
} // namespace metop