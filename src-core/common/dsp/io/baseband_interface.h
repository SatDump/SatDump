#pragma once

#include <fstream>
#include <filesystem>
#include "common/dsp/buffer.h"
#include "common/utils.h"
#include "common/dsp/complex.h"
#include "common/wav.h"
#ifdef BUILD_ZIQ
#include "common/ziq.h"
#endif
#include "common/ziq2.h"
#include "wav_writer.h"

namespace dsp
{
    enum BasebandType
    {
        NONE,
        CF_32,
        IS_16,
        IS_8,
        IU_8,
        WAV_16,
#ifdef BUILD_ZIQ
        ZIQ,
#endif
        ZIQ2,
    };

    BasebandType basebandTypeFromString(std::string type);

    // "Simple" class to wrap all the baseband reading stuff, including sample scaling. All inline for performance
    class BasebandReader
    {
    public:
        // File stuff
        uint64_t filesize;
        uint64_t progress;
        std::ifstream input_file;

        // Control
        bool should_repeat = false;

    private:
        // Main mutex
        std::mutex main_mtx;

        // Format
        BasebandType format;

        // Buffers
        int16_t *buffer_i16;
        int8_t *buffer_i8;
        uint8_t *buffer_u8;

#ifdef BUILD_ZIQ
        std::shared_ptr<ziq::ziq_reader> ziqReader;
#endif

        // Wav handling
        bool is_wav = false;
        bool is_rf64 = false;

    public:
        BasebandReader()
        {
            buffer_i16 = create_volk_buffer<int16_t>(STREAM_BUFFER_SIZE * 2);
            buffer_i8 = create_volk_buffer<int8_t>(STREAM_BUFFER_SIZE * 2);
            buffer_u8 = create_volk_buffer<uint8_t>(STREAM_BUFFER_SIZE * 2);
        }

        ~BasebandReader()
        {
            volk_free(buffer_i16);
            volk_free(buffer_i8);
            volk_free(buffer_u8);
        }

        // Set the file you want to work on
        inline void set_file(std::string file_path, BasebandType format)
        {
            main_mtx.lock();
            if (std::filesystem::is_fifo(file_path))
            {
                filesize = 1;
                progress = 0;
            }
            else
            {
                filesize = getFilesize(file_path);
                progress = 0;

                is_wav |= wav::isValidWav(wav::parseHeaderFromFileWav(file_path));
                is_rf64 |= wav::isValidRF64(wav::parseHeaderFromFileWav(file_path));
            }

            this->format = format;
            input_file = std::ifstream(file_path, std::ios::binary);

#ifdef BUILD_ZIQ
            if (format == ZIQ)
                ziqReader = std::make_shared<ziq::ziq_reader>(input_file);
#endif

            if (format == ZIQ2)
                input_file.seekg(4);
            else if (is_wav)
                input_file.seekg(sizeof(wav::WavHeader));
            else if (is_rf64)
                input_file.seekg(sizeof(wav::RF64Header));

            main_mtx.unlock();
        }

        inline int read_samples(complex_t *output_buffer, int buffer_size)
        {
            main_mtx.lock();
            if (should_repeat && input_file.eof())
            {
                input_file.clear();
#ifdef BUILD_ZIQ
                if (format == ZIQ)
                    ziqReader->seekg(0);
                else
#endif
#ifdef BUILD_ZIQ2
                    if (format == ZIQ2)
                    input_file.seekg(4);
                else
#endif
                    input_file.seekg(0);
            }

            switch (format)
            {
            case CF_32:
                input_file.read((char *)output_buffer, buffer_size * sizeof(complex_t));
                break;

            case WAV_16:
            case IS_16:
                input_file.read((char *)buffer_i16, buffer_size * sizeof(int16_t) * 2);
                volk_16i_s32f_convert_32f_u((float *)output_buffer, (const int16_t *)buffer_i16, 65535, buffer_size * 2);
                break;

            case IS_8:
                input_file.read((char *)buffer_i8, buffer_size * sizeof(int8_t) * 2);
                volk_8i_s32f_convert_32f_u((float *)output_buffer, (const int8_t *)buffer_i8, 127, buffer_size * 2);
                break;

            case IU_8:
                input_file.read((char *)buffer_u8, buffer_size * sizeof(uint8_t) * 2);
                for (int i = 0; i < buffer_size; i++)
                {
                    float imag = (buffer_u8[i * 2 + 1] - 127) * (1.0 / 127.0);
                    float real = (buffer_u8[i * 2 + 0] - 127) * (1.0 / 127.0);
                    output_buffer[i] = complex_t(real, imag);
                }
                break;

#ifdef BUILD_ZIQ
            case ZIQ:
                ziqReader->read(output_buffer, buffer_size);
                break;
#endif
#ifdef BUILD_ZIQ2
            case ZIQ2:
            {
                input_file.read((char *)buffer_u8, 4 + sizeof(ziq2::ziq2_pkt_hdr_t));
                ziq2::ziq2_pkt_hdr_t *mdr = (ziq2::ziq2_pkt_hdr_t *)&buffer_u8[4];
                input_file.read((char *)&buffer_u8[4 + sizeof(ziq2::ziq2_pkt_hdr_t)], mdr->pkt_size);

                if (mdr->pkt_type == ziq2::ZIQ2_PKT_IQ)
                    ziq2::ziq2_read_iq_pkt(&buffer_u8[4], output_buffer, &buffer_size);
                else
                    buffer_size = 0;
            }
            break;
#endif

            default:
                break;
            }

            // if (is_wav)
            //     for (int i = 0; i < buffer_size; i++)
            //         output_buffer[i] = complex_t(output_buffer[i].imag, output_buffer[i].real);

            progress = input_file.tellg();

            main_mtx.unlock();

            return buffer_size;
        }

        inline void set_progress(uint64_t progress)
        {
#ifdef BUILD_ZIQ
            if (format == ZIQ)
            {
                main_mtx.lock();
                ziqReader->seekg(filesize * (progress / 100.0f));
                main_mtx.unlock();
                return;
            }
#endif

            if (format == ZIQ2)
            {
                main_mtx.lock();
                uint64_t pos = filesize * (progress / 100.0f);
                uint8_t sync[4];
                while (pos < filesize)
                {
                    input_file.seekg(pos);
                    input_file.read((char *)sync, 4);
                    if (sync[0] == 0x1a && sync[1] == 0xcf && sync[2] == 0xfc && sync[3] == 0x1d)
                        break;
                    pos++;
                }
                input_file.seekg(pos);
                main_mtx.unlock();
                return;
            }

            main_mtx.lock();

            int samplesize = sizeof(complex_t);

            switch (format)
            {
            case CF_32:
                samplesize = sizeof(complex_t);
                break;

            case WAV_16:
            case IS_16:
                samplesize = sizeof(int16_t) * 2;
                break;

            case IS_8:
                samplesize = sizeof(int8_t) * 2;
                break;

            case IU_8:
                samplesize = sizeof(uint8_t) * 2;
                break;

            default:
                break;
            }

            uint64_t position = double(filesize / samplesize - 1) * (progress / 100.0f);
            input_file.seekg(position * samplesize);
            main_mtx.unlock();
        }

        inline void close()
        {
            input_file.close();
        }

        inline bool is_eof()
        {
            if (input_file.is_open())
                return input_file.eof();
            else
                return false;
        }
    };

    class BasebandWriter
    {
    private:
        std::mutex rec_mutex;

        BasebandType d_sample_format;

        std::ofstream output_file;

        uint64_t current_size_out = 0;
        uint64_t current_size_out_raw = 0;

        int8_t *buffer_s8;
        int16_t *buffer_s16;

        int bit_depth = 0;

#ifdef BUILD_ZIQ
        ziq::ziq_cfg ziqcfg;
        std::shared_ptr<ziq::ziq_writer> ziqWriter;
#endif

        float *mag_buffer = nullptr;

        std::unique_ptr<WavWriter> wav_writer;

        bool should_work = false;

    public:
        BasebandWriter()
        {
            buffer_s8 = create_volk_buffer<int8_t>(STREAM_BUFFER_SIZE * 2);
            buffer_s16 = create_volk_buffer<int16_t>(STREAM_BUFFER_SIZE * 2);
        }

        ~BasebandWriter()
        {
            volk_free(buffer_s8);
            volk_free(buffer_s16);
            if (mag_buffer != nullptr)
                volk_free(mag_buffer);
        }

        void set_output_sample_type(BasebandType sample_format)
        {
            d_sample_format = sample_format;
        }

        std::string start_recording(std::string path_without_ext, uint64_t samplerate, int depth = 0, bool override_filename = false) // Depth is only for compressed non-raw formats
        {
            rec_mutex.lock();

            bit_depth = depth;

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
            else if (d_sample_format == ZIQ2)
                finalt = path_without_ext + ".ziq";

            if (override_filename)
                finalt = path_without_ext;

            current_size_out = 0;
            current_size_out_raw = 0;

            output_file = std::ofstream(finalt, std::ios::binary);

            if (d_sample_format == WAV_16)
            {
                wav_writer = std::make_unique<WavWriter>(output_file);
                wav_writer->write_header(samplerate, 2);
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
#ifdef BUILD_ZIQ2
            if (d_sample_format == ZIQ2)
            {
                int sz = ziq2::ziq2_write_file_hdr((uint8_t *)buffer_s8, samplerate);
                output_file.write((char *)buffer_s8, sz);

                if (mag_buffer == nullptr)
                    mag_buffer = create_volk_buffer<float>(STREAM_BUFFER_SIZE);
            }
#endif

            should_work = true;
            rec_mutex.unlock();

            return finalt;
        }

        uint64_t get_written()
        {
            return current_size_out;
        }

        uint64_t get_written_raw()
        {
            return current_size_out_raw;
        }

        void stop_recording()
        {
            if (d_sample_format == WAV_16)
                wav_writer->finish_header(get_written());

            rec_mutex.lock();
            should_work = false;
            current_size_out = 0;
            current_size_out_raw = 0;
            output_file.close();
            rec_mutex.unlock();
        }

        void feed_samples(complex_t *samples, int nsamples)
        {
            if (nsamples <= 0 || !should_work)
                return;

            rec_mutex.lock();
            if (should_work)
            {
                if (d_sample_format == CF_32)
                {
                    output_file.write((char *)samples, nsamples * sizeof(complex_t));
                    current_size_out += nsamples * sizeof(complex_t);
                }
                else if (d_sample_format == IS_16 || d_sample_format == WAV_16)
                {
                    volk_32f_s32f_convert_16i(buffer_s16, (float *)samples, 65535, nsamples * 2);
                    output_file.write((char *)buffer_s16, nsamples * sizeof(int16_t) * 2);
                    current_size_out += nsamples * sizeof(int16_t) * 2;
                }
                else if (d_sample_format == IS_8)
                {
                    volk_32f_s32f_convert_8i(buffer_s8, (float *)samples, 127, nsamples * 2);
                    output_file.write((char *)buffer_s8, nsamples * sizeof(int8_t) * 2);
                    current_size_out += nsamples * sizeof(int8_t) * 2;
                }
#ifdef BUILD_ZIQ
                else if (d_sample_format == ZIQ)
                {
                    current_size_out += ziqWriter->write(samples, nsamples);
                    current_size_out_raw += (ziqcfg.bits_per_sample / 4) * nsamples;
                }
#endif
#ifdef BUILD_ZIQ2
                else if (d_sample_format == ZIQ2)
                {
                    int sz = ziq2::ziq2_write_iq_pkt((uint8_t *)buffer_s8, samples, mag_buffer, nsamples, bit_depth);
                    output_file.write((char *)buffer_s8, sz);
                    current_size_out += sz;
                }
#endif

                output_file.flush();
            }

            rec_mutex.unlock();
        }
    };
}