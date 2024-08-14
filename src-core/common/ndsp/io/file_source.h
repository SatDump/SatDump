#pragma once

#include "common/dsp/complex.h"
#include "../block.h"

#include "common/dsp/io/baseband_interface.h"

namespace ndsp
{
    class FileSource : public ndsp::Block
    {
    private:
        void work();

        dsp::BasebandType type;
        int buffer_size;
        bool iq_swap;
        std::atomic<uint64_t> d_filesize;
        std::atomic<uint64_t> d_progress;
        std::atomic<bool> d_eof;

        dsp::BasebandReader baseband_reader;

    public:
        std::string d_file = "";
        dsp::BasebandType d_type;
        int d_buffer_size = 8192;
        bool d_iq_swap = false;

    public:
        FileSource();

        void set_params(nlohmann::json p = {});
        void start();
        void stop();

        bool eof() { return d_eof; }
        uint64_t filesize() { return d_filesize; }
        uint64_t position() { return d_progress; }
    };
}