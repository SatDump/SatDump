#pragma once

#include "block.h"
#include <fstream>
#include <atomic>
//#define ENABLE_DECOMPRESSION
#ifdef ENABLE_DECOMPRESSION
#include <volk/volk_alloc.hh>
#include <zstd.h>
#endif

namespace dsp
{
#ifdef ENABLE_DECOMPRESSION
    class StreamingDecompressor
    {
    private:
        ZSTD_DCtx *zstd_ctx;
        ZSTD_inBuffer zstd_input;
        ZSTD_outBuffer zstd_output;

    public:
        StreamingDecompressor()
        {
            zstd_ctx = ZSTD_createDCtx();
        }

        ~StreamingDecompressor()
        {
            ZSTD_freeDCtx(zstd_ctx);
        }

        int work(uint8_t *input, int size, uint8_t *output)
        {
            zstd_input = {input, (unsigned long)size, 0};
            zstd_output = {output, (unsigned long)size * 100, 0};

            while (zstd_input.pos < zstd_input.size)
            {

                size_t t = ZSTD_decompressStream(zstd_ctx, &zstd_output, &zstd_input);
                if (ZSTD_isError(t))
                {
                    ZSTD_DCtx_reset(zstd_ctx, ZSTD_reset_session_only);
                    return 0;
                }
            }

            return zstd_output.pos;
        }
    };
#endif

    enum BasebandType
    {
        COMPLEX_FLOAT_32,
        INTEGER_16,
        INTEGER_8,
        WAV_8
    };

    BasebandType BasebandTypeFromString(std::string type);

    class FileSourceBlock : public Block<uint8_t, complex_t>
    {
    private:
        dsp::stream<uint8_t> dummystream;

        std::ifstream d_input_file;
        const BasebandType d_type;
        std::atomic<uint64_t> d_filesize;
        std::atomic<uint64_t> d_progress;
        const int d_buffer_size;
        const bool d_iq_swap;
        std::atomic<bool> d_eof;
        void work();
        uint64_t getFilesize(std::string filepath);
        // Int16 buffer
        int16_t *buffer_i16;
        // Int8 buffer
        int8_t *buffer_i8;
        // Uint8 buffer
        uint8_t *buffer_u8;

#ifdef ENABLE_DECOMPRESSION
        uint8_t *compressedBuffer;
        StreamingDecompressor decompressor;
        volk::vector<uint8_t> decompressionVector;
#endif

    public:
        FileSourceBlock(std::string file, BasebandType type, int buffer_size, bool iq_swap = false);
        ~FileSourceBlock();
        uint64_t getFilesize();
        uint64_t getPosition();
        bool eof();
    };
}