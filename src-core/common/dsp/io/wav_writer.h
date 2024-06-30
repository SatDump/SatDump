#pragma once

#include <fstream>

namespace dsp
{
    class WavWriter
    {
    private:
        std::ofstream &data_out;
        long samplerate = 0;
        int channels = 0;

    public:
        WavWriter(std::ofstream &fout)
            : data_out(fout)
        {
        }

        void write_header(long samplerate, int channels, bool rf64 = false, uint64_t final_size = 0)
        {
            this->samplerate = samplerate;
            this->channels = channels;

            if (rf64)
            {
                // write rf64 header
                uint32_t junk_size = 28;
                uint32_t dummy_size = -1;
                uint32_t subchunk1_size = 16;
                uint16_t audio_format = 1;
                uint16_t num_channels = channels;
                uint16_t bits_per_sample = 16;
                uint32_t byte_rate = samplerate * num_channels * bits_per_sample / 8;
                uint16_t block_align = num_channels * bits_per_sample / 8;
                uint32_t wav_samplerate = samplerate;

                uint32_t rf_size_low = (final_size + 72) & 0xffffffff;
                uint32_t rf_size_high = ((final_size + 72) / 4294967296) & 0xffffffff;
                uint32_t data_size_low = final_size & 0xffffffff;
                uint32_t data_size_high = (final_size / 4294967296) & 0xffffffff;
                uint32_t sample_count_low = (final_size / 4) & 0xffffffff;
                uint32_t sample_count_high = ((final_size / 4) / 4294967296) & 0xffffffff;
                uint32_t table_size = 0;

                data_out.write("RF64----WAVE", 12);
                data_out.write("ds64", 4);
                data_out.write((char *)&junk_size, 4);
                data_out.write((char *)&rf_size_low, 4);
                data_out.write((char *)&rf_size_high, 4);
                data_out.write((char *)&data_size_low, 4);
                data_out.write((char *)&data_size_high, 4);
                data_out.write((char *)&sample_count_low, 4);
                data_out.write((char *)&sample_count_high, 4);
                data_out.write((char *)&table_size, 4);
                data_out.write("fmt ", 4);
                data_out.write((char *)&subchunk1_size, 4);
                data_out.write((char *)&audio_format, 2);
                data_out.write((char *)&num_channels, 2);
                data_out.write((char *)&wav_samplerate, 4);
                data_out.write((char *)&byte_rate, 4);
                data_out.write((char *)&block_align, 2);
                data_out.write((char *)&bits_per_sample, 2);
                data_out.write("data----", 8);

                // Change header to RF64
                data_out.seekp(0);
                data_out.write("RF64", 4);

                // Set 32 bit sizes to -1
                data_out.write((char *)&dummy_size, 4);
                data_out.seekp(76);
                data_out.write((char *)&dummy_size, 4);
            }
            else
            {
                // write wav header
                uint32_t subchunk1_size = 16;
                uint16_t audio_format = 1;
                uint16_t num_channels = channels;
                uint16_t bits_per_sample = 16;
                uint32_t byte_rate = samplerate * num_channels * bits_per_sample / 8;
                uint16_t block_align = num_channels * bits_per_sample / 8;
                uint32_t wav_samplerate = samplerate;

                data_out.write("RIFF----WAVE", 12);
                data_out.write("fmt ", 4);
                data_out.write((char *)&subchunk1_size, 4);
                data_out.write((char *)&audio_format, 2);
                data_out.write((char *)&num_channels, 2);
                data_out.write((char *)&wav_samplerate, 4);
                data_out.write((char *)&byte_rate, 4);
                data_out.write((char *)&block_align, 2);
                data_out.write((char *)&bits_per_sample, 2);
                data_out.write("data----", 8);
            }
        }

        void finish_header(uint64_t final_size)
        {
            // 4294967259 is max size of standard WAV with minimal headers
            // By adding the JUNK section this becomes 4294967223
            if (final_size <= 4294967259)
            {
                uint32_t data_size = final_size;
                uint32_t chunk_size = data_size + 36;

                data_out.seekp(4);
                data_out.write((char *)&chunk_size, 4);
                data_out.seekp(40);
                data_out.write((char *)&data_size, 4);
            }
            else
            {
                data_out.seekp(0);
                write_header(samplerate, channels, true, final_size);
            }
        }
    };
};