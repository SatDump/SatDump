#pragma once

#include "common/dsp/block.h"
#include <fstream>
#include <atomic>
#include "baseband_interface.h"

namespace dsp
{
    class FileSourceBlock : public Block<uint8_t, complex_t>
    {
    private:
        dsp::stream<uint8_t> dummystream;

        const BasebandType d_type;
        std::atomic<uint64_t> d_filesize;
        std::atomic<uint64_t> d_progress;
        const int d_buffer_size;
        const bool d_iq_swap;
        std::atomic<bool> d_eof;
        void work();

        BasebandReader baseband_reader;

    public:
        FileSourceBlock(std::string file, BasebandType type, int buffer_size, bool iq_swap = false);
        ~FileSourceBlock();
        uint64_t getFilesize();
        uint64_t getPosition();
        bool eof();
    };
}