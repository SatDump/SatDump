#pragma once

#include "block.h"
#include "file_source.h"
#ifdef BUILD_ZIQ
#include "common/ziq.h"
#endif

namespace dsp
{
    class FileSinkBlock : public Block<complex_t, float>
    {
    private:
        std::mutex rec_mutex;

        BasebandType d_sample_format;
        void work();

        bool should_work = false;

        std::ofstream output_file;

        size_t current_size_out = 0;
        size_t current_size_out_raw = 0;

        int8_t *buffer_s8;
        int16_t *buffer_s16;

#ifdef BUILD_ZIQ
        ziq::ziq_cfg ziqcfg;
        std::shared_ptr<ziq::ziq_writer> ziqWriter;
#endif

    public:
        FileSinkBlock(std::shared_ptr<dsp::stream<complex_t>> input);
        ~FileSinkBlock();

        void set_output_sample_type(BasebandType sample_format)
        {
            d_sample_format = sample_format;
        }

        std::string start_recording(std::string path_without_ext, uint64_t samplerate, int depth = 0, bool override_filename = false) // Depth is only for compressed non-raw formats
        {
            rec_mutex.lock();
            std::string finalt;
            if (d_sample_format == CF_32)
                finalt = path_without_ext + ".f32";
            else if (d_sample_format == IS_16)
                finalt = path_without_ext + ".s16";
            else if (d_sample_format == IS_8)
                finalt = path_without_ext + ".s8";
            else if (d_sample_format == WAV_16)
                finalt = path_without_ext + ".wav";
#ifdef BUILD_ZIQ
            else if (d_sample_format == ZIQ)
                finalt = path_without_ext + ".ziq";
#endif

            if (override_filename)
                finalt = path_without_ext;

            current_size_out = 0;
            current_size_out_raw = 0;

            output_file = std::ofstream(finalt, std::ios::binary);

            if (d_sample_format == WAV_16)
            {
                // write wav header
                uint32_t subchunk1_size = 16;
                uint16_t audio_format = 1;
                uint16_t num_channels = 2;
                uint16_t bits_per_sample = 16;
                uint32_t byte_rate = samplerate * num_channels * bits_per_sample / 8;
                uint16_t block_align = num_channels * bits_per_sample / 8;
                uint32_t wav_samplerate = samplerate;

                output_file.write("RIFF----WAVE", 12);
                output_file.write("fmt ", 4);
                output_file.write((char *)&subchunk1_size, 4);
                output_file.write((char *)&audio_format, 2);
                output_file.write((char *)&num_channels, 2);
                output_file.write((char *)&wav_samplerate, 4);
                output_file.write((char *)&byte_rate, 4);
                output_file.write((char *)&block_align, 2);
                output_file.write((char *)&bits_per_sample, 2);
                output_file.write("data----", 8);
            }

#ifdef BUILD_ZIQ
            if (d_sample_format == ZIQ)
            {
                ziqcfg.is_compressed = true;
                ziqcfg.bits_per_sample = depth;
                ziqcfg.samplerate = samplerate;
                ziqcfg.annotation = "";

                ziqWriter = std::make_shared<ziq::ziq_writer>(ziqcfg, output_file);
            }
#endif

            should_work = true;
            rec_mutex.unlock();

            return finalt;
        }

        size_t get_written()
        {
            return current_size_out;
        }

        size_t get_written_raw()
        {
            return current_size_out_raw;
        }

        void stop_recording()
        {
            if (d_sample_format == WAV_16)
            {
                // complete wav header
                uint32_t data_size = get_written();
                uint32_t chunk_size = data_size + 36;

                output_file.seekp(4);
                output_file.write((char *)&chunk_size, 4);
                output_file.seekp(40);
                output_file.write((char *)&data_size, 4);
            }

            rec_mutex.lock();
            should_work = false;
            current_size_out = 0;
            current_size_out_raw = 0;
            output_file.close();
            rec_mutex.unlock();
        }
    };
}