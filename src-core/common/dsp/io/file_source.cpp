#include "file_source.h"
#include <volk/volk.h>

namespace dsp
{
    FileSourceBlock::FileSourceBlock(std::string file, BasebandType type, int buffer_size, bool iq_swap) : Block(nullptr),
                                                                                                           d_type(type),
                                                                                                           d_buffer_size(buffer_size),
                                                                                                           d_iq_swap(iq_swap)
    {
        baseband_reader.set_file(file, type);

        d_filesize = baseband_reader.filesize;
        d_progress = 0;
        d_eof = false;

        d_got_input = false;
    }

    FileSourceBlock::~FileSourceBlock()
    {
        baseband_reader.close();
    }

    void FileSourceBlock::work()
    {
        if (!baseband_reader.is_eof())
        {
            int read = baseband_reader.read_samples(output_stream->writeBuf, d_buffer_size);

            if (d_iq_swap)
                for (int i = 0; i < read; i++)
                    output_stream->writeBuf[i] = complex_t(output_stream->writeBuf[i].imag, output_stream->writeBuf[i].real);

            output_stream->swap(read);

            d_progress = baseband_reader.progress;
        }
        else
        {
            d_eof = true;
        }
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
}