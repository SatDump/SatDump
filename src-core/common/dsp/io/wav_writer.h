#pragma once

#include <fstream>

namespace dsp
{
    class WavWriter
    {
    private:
        std::ofstream &data_out;

    public:
        WavWriter(std::ofstream &fout)
            : data_out(fout)
        {
        }

        void write_header(long samplerate, int channels)
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

        void finish_header(size_t final_size)
        {
            uint32_t data_size = final_size;
            uint32_t chunk_size = data_size + 36;

            data_out.seekp(4);
            data_out.write((char *)&chunk_size, 4);
            data_out.seekp(40);
            data_out.write((char *)&data_size, 4);
        }
    };
};