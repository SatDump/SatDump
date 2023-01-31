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
    };

    BasebandType basebandTypeFromString(std::string type);

    // "Simple" class to wrap all the baseband reading stuff, including sample scaling. All inline for performance
    class BasebandReader
    {
    public:
        // File stuff
        size_t filesize;
        size_t progress;
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
                is_wav |= wav::isValidRF64(wav::parseHeaderFromFileWav(file_path));
            }

            this->format = format;
            input_file = std::ifstream(file_path, std::ios::binary);

#ifdef BUILD_ZIQ
            if (format == ZIQ)
                ziqReader = std::make_shared<ziq::ziq_reader>(input_file);
#endif

            main_mtx.unlock();
        }

        inline int read_samples(complex_t *output_buffer, int buffer_size)
        {
            main_mtx.lock();
            if (should_repeat && input_file.eof())
            {
                input_file.clear();
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

        inline void set_progress(size_t progress)
        {
#ifdef BUILD_ZIQ
            if (format == ZIQ)
                return;
#endif

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
}