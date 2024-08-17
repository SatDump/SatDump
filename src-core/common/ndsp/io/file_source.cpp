#include "file_source.h"

namespace ndsp
{
    FileSource::FileSource()
        : ndsp::Block("file_source", {}, {{sizeof(complex_t)}})
    {
        d_eof = false;
    }

    void FileSource::start()
    {
        set_params();
        ndsp::buf::init_nafifo_stdbuf<complex_t>(outputs[0], 2, buffer_size);
        ndsp::Block::start();
    }

    void FileSource::stop()
    {
        ndsp::Block::stop();
        baseband_reader.close();
    }

    void FileSource::set_params(nlohmann::json p)
    {
        if (p.contains("type"))
            d_type = p["type"];
        if (p.contains("file"))
            d_file = p["file"];
        if (p.contains("buffer_size"))
            d_buffer_size = p["buffer_size"];
        if (p.contains("freq_limit"))
            iq_swap = p["iq_swap"];

        if (!is_running())
        {
            type = d_type;
            buffer_size = d_buffer_size;

            baseband_reader.set_file(d_file, type);
            d_filesize = baseband_reader.filesize;
            d_progress = 0;
            d_eof = false;
        }

        iq_swap = d_iq_swap;
    }

    void FileSource::work()
    {
        auto *wbuf = (ndsp::buf::StdBuf<complex_t> *)outputs[0]->write_buf();

        if (!baseband_reader.is_eof())
        {
            int read = baseband_reader.read_samples(wbuf->dat, buffer_size);

            if (d_iq_swap)
                for (int i = 0; i < read; i++)
                    wbuf->dat[i] = complex_t(wbuf->dat[i].imag, wbuf->dat[i].real);

            wbuf->cnt = read;
            outputs[0]->write();

            d_progress = baseband_reader.progress;
        }
        else
        {
            d_eof = true;
        }
    }
}
