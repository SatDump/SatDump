#include "file_source.h"
#include <volk/volk.h>

namespace dsp
{
    FileSourceBlock::FileSourceBlock(std::string file, BasebandType type, int buffer_size) : Block(nullptr), d_type(type), d_buffer_size(buffer_size)
    {
        d_filesize = getFilesize(file);
        d_progress = 0;
        d_eof = false;

        d_input_file = std::ifstream(file, std::ios::binary);

        buffer_i16 = new int16_t[d_buffer_size * 2];
        buffer_i8 = new int8_t[d_buffer_size * 2];
        buffer_u8 = new uint8_t[d_buffer_size * 2];

        d_got_input = false;
    }

    FileSourceBlock::~FileSourceBlock()
    {
        delete[] buffer_i16;
        delete[] buffer_i8;
        delete[] buffer_u8;

        d_input_file.close();
    }

    void FileSourceBlock::work()
    {
        if (!d_input_file.eof())
        {
            // Get baseband, possibly convert to F32
            switch (d_type)
            {
            case COMPLEX_FLOAT_32:
                d_input_file.read((char *)output_stream->writeBuf, d_buffer_size * sizeof(std::complex<float>));
                break;

            case INTEGER_16:
                d_input_file.read((char *)buffer_i16, d_buffer_size * sizeof(int16_t) * 2);
                for (int i = 0; i < d_buffer_size; i++)
                    output_stream->writeBuf[i] = std::complex<float>(buffer_i16[i * 2] * 0.00004f, (float)buffer_i16[i * 2 + 1] * 0.00004f);
                // volk_16i_s32f_convert_32f_u((float *)output_stream->writeBuf, (const int16_t *)buffer_i16, 1.0f / 0.00004f, d_buffer_size * 2);
                break;

            case INTEGER_8:
                d_input_file.read((char *)buffer_i8, d_buffer_size * sizeof(int8_t) * 2);
                for (int i = 0; i < d_buffer_size; i++)
                    output_stream->writeBuf[i] = std::complex<float>(buffer_i8[i * 2] * 0.004f, (float)buffer_i8[i * 2 + 1] * 0.004f);
                // volk_8i_s32f_convert_32f_u((float *)output_stream->writeBuf, (const int8_t *)buffer_i8, 1.0f / 0.004f, d_buffer_size * 2);
                break;

            case WAV_8:
                d_input_file.read((char *)buffer_u8, d_buffer_size * sizeof(uint8_t) * 2);
                for (int i = 0; i < d_buffer_size; i++)
                {
                    float imag = (buffer_u8[i * 2] - 127) * 0.004f;
                    float real = (buffer_u8[i * 2 + 1] - 127) * 0.004f;
                    output_stream->writeBuf[i] = std::complex<float>(real, imag);
                }
                break;

            default:
                break;
            }

            output_stream->swap(d_buffer_size);

            d_progress = d_input_file.tellg();
        }
        else
        {
            d_eof = true;
        }
    }

    uint64_t FileSourceBlock::getFilesize(std::string filepath)
    {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        std::uint64_t fileSize = file.tellg();
        file.close();
        return fileSize;
    }

    uint64_t FileSourceBlock::getFilesize()
    {
        return d_filesize;
    }
    uint64_t FileSourceBlock::getPosition()
    {
        return d_progress;
    }
    bool FileSourceBlock::eof()
    {
        return d_eof;
    }

    BasebandType BasebandTypeFromString(std::string type)
    {
        if (type == "i16")
            return INTEGER_16;
        else if (type == "i8")
            return INTEGER_8;
        else if (type == "f32")
            return COMPLEX_FLOAT_32;
        else if (type == "w8")
            return WAV_8;
    }
}