#pragma once

#include "block.h"
#include <dsp/agc.h>
#include <fstream>
#include <atomic>

namespace dsp
{
    enum BasebandType
    {
        COMPLEX_FLOAT_32,
        INTEGER_16,
        INTEGER_8,
        WAV_8
    };

    BasebandType BasebandTypeFromString(std::string type);

    class FileSourceBlock : public Block<uint8_t, std::complex<float>>
    {
    private:
        dsp::stream<uint8_t> dummystream;

        std::ifstream d_input_file;
        const BasebandType d_type;
        std::atomic<size_t> d_filesize;
        std::atomic<size_t> d_progress;
        const int d_buffer_size;
        std::atomic<bool> d_eof;
        void work();
        size_t getFilesize(std::string filepath);
        // Int16 buffer
        int16_t *buffer_i16;
        // Int8 buffer
        int8_t *buffer_i8;
        // Uint8 buffer
        uint8_t *buffer_u8;

    public:
        FileSourceBlock(std::string file, BasebandType type, int buffer_size);
        ~FileSourceBlock();
        size_t getFilesize();
        size_t getPosition();
        bool eof();
    };
}