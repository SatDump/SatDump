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
#include "baseband_type.h"

namespace dsp
{
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
        int32_t *buffer_s32 = nullptr;
        int16_t *buffer_s16 = nullptr;
        int8_t *buffer_s8 = nullptr;
        uint8_t *buffer_u8 = nullptr;

#ifdef BUILD_ZIQ
        std::shared_ptr<ziq::ziq_reader> ziqReader;
#endif

        // Wav handling
        bool is_wav = false;
        bool is_rf64 = false;

    public:
        BasebandReader()
        {
        }

        ~BasebandReader()
        {
            if (buffer_s8 != nullptr)
                volk_free(buffer_s8);
            if (buffer_s16 != nullptr)
                volk_free(buffer_s16);
            if (buffer_s32 != nullptr)
                volk_free(buffer_s32);
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

            if (this->format != format)
            {
                // Free old buffer
                if (this->format == CS_8 && buffer_s8 != nullptr)
                {
                    volk_free(buffer_s8);
                    buffer_s8 = nullptr;
                }
                else if (this->format == CU_8 && buffer_u8 != nullptr)
                {
                    volk_free(buffer_u8);
                    buffer_u8 = nullptr;
                }
                else if ((this->format == CS_16 || this->format == WAV_16) && buffer_s16 != nullptr)
                {
                    volk_free(buffer_s16);
                    buffer_s16 = nullptr;
                }
                else if (this->format == CS_32 && buffer_s32 != nullptr)
                {
                    volk_free(buffer_s32);
                    buffer_s32 = nullptr;
                }
#ifdef BUILD_ZIQ2
                else if (this->format == ZIQ2 && buffer_s8 != nullptr)
                {
                    volk_free(buffer_s8);
                    buffer_s8 = nullptr;
                }
#endif

                // Alloc new buffer
                if (format == CS_8)
                    buffer_s8 = create_volk_buffer<int8_t>(STREAM_BUFFER_SIZE * 2);
                else if (format == CU_8)
                    buffer_u8 = create_volk_buffer<uint8_t>(STREAM_BUFFER_SIZE * 2);
                else if (format == CS_16 || format == WAV_16)
                    buffer_s16 = create_volk_buffer<int16_t>(STREAM_BUFFER_SIZE * 2);
                else if (format == CS_32)
                    buffer_s32 = create_volk_buffer<int32_t>(STREAM_BUFFER_SIZE * 2);
#ifdef BUILD_ZIQ2
                else if (format == ZIQ2)
                    buffer_s8 = create_volk_buffer<int8_t>(STREAM_BUFFER_SIZE * 2);
#endif
                this->format = format;
            }

            input_file = std::ifstream(file_path, std::ios::binary);

#ifdef BUILD_ZIQ
            if (format == ZIQ)
                ziqReader = std::make_shared<ziq::ziq_reader>(input_file);
            else
#endif
#ifdef BUILD_ZIQ2
            if (format == ZIQ2)
                input_file.seekg(4);
            else
#endif
            if (is_wav)
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

            case CS_32:
                input_file.read((char*)buffer_s32, buffer_size * sizeof(int32_t) * 2);
                volk_32i_s32f_convert_32f_u((float *)output_buffer, (const int32_t*)buffer_s32, 2147483647, buffer_size * 2);
                break;

            case WAV_16: case CS_16:
                input_file.read((char*)buffer_s16, buffer_size * sizeof(int16_t) * 2);
                volk_16i_s32f_convert_32f_u((float *)output_buffer, (const int16_t*)buffer_s16, 32767, buffer_size * 2);
                break;

            case CS_8:
                input_file.read((char *)buffer_s8, buffer_size * sizeof(int8_t) * 2);
                volk_8i_s32f_convert_32f_u((float *)output_buffer, (const int8_t *)buffer_s8, 127, buffer_size * 2);
                break;

            case CU_8:
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
#ifdef BUILD_ZIQ2
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
#endif
            main_mtx.lock();

            int samplesize = sizeof(complex_t);

            switch (format)
            {
            case CF_32:
                samplesize = sizeof(complex_t);
                break;

            case CS_32:
                samplesize = sizeof(int32_t) * 2;
                break;

            case WAV_16:
            case CS_16:
                samplesize = sizeof(int16_t) * 2;
                break;

            case CS_8:
                samplesize = sizeof(int8_t) * 2;
                break;

            case CU_8:
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
