#pragma once

#ifdef BUILD_ZIQ
#include <string>
#include <cstdint>
#include <fstream>
#include <zstd.h>
#include "dsp/complex.h"

/*
ZIQ - A Simple, efficient baseband format supporting ZSTD-based
compression.
This started with the problem of high samplerate recording
taking up a lot of disk space. ZSTD-based live compression
was implemented providing disk space savings from 1.5 to 4x
compared to raw, uncompressed IQ data.
ZST was chosen as a compression standard over zlib, lzo, and
others due to it's CPU effiency. A few threads can do 1GB/s
compression on any decently recent machine, and much more in
decompression. Hence, the impact of using compression is kept
very low.

Supported sample formats are :
- int8_t
- int16_t
- float (32)
*/

// File signature of a ZIQ File
#define ZIQ_SIGNATURE "ZIQ_"

namespace ziq
{
    // Struct holding the parameters of a ZIQ Baseband (header)
    struct ziq_cfg
    {
        bool is_compressed;     // If the data is compressed or not
        char bits_per_sample;   // Bits per sample
        uint64_t samplerate;    // Samplerate of the contained IQ data
        std::string annotation; // Annotation field. This should be JSON and can used to store frequency and other informations
    };

    class ziq_writer
    {
    private:
        ziq_cfg cfg;
        std::ofstream &stream;
        int8_t *buffer_i8;
        int16_t *buffer_i16;

    private:
        const int zst_level = 1;
        const int zst_workers = 8;
        ZSTD_CCtx *zstd_ctx;
        ZSTD_inBuffer zstd_input;
        ZSTD_outBuffer zstd_output;
        int zst_outc;
        size_t max_buffer_size;
        uint8_t *output_compressed;

    private:
        int compress_and_write(uint8_t *input, size_t size);

    public:
        ziq_writer(ziq_cfg cfg, std::ofstream &stream);
        ~ziq_writer();

        int write(complex_t *input, int size);
    };

    class ziq_reader
    {
    private:
        bool isValid;
        ziq_cfg cfg;
        std::ifstream &stream;
        int8_t *buffer_i8;
        int16_t *buffer_i16;
        uint64_t annotation_size;

    private:
        ZSTD_DCtx *zstd_ctx;
        ZSTD_inBuffer zstd_input;
        ZSTD_outBuffer zstd_output;
        int zst_outc;
        size_t max_buffer_size;
        uint8_t *compressed_buffer;
        int decompressed_cnt;
        uint8_t *output_decompressed;

    private:
        int decompress_at_least(int size);
        int read_decompressed(uint8_t *buffer, int size);

    public:
        ziq_reader(std::ifstream &stream);
        ~ziq_reader();

        int read(complex_t *output, int size);
        bool seekg(size_t pos);
    };

    bool isValidZIQ(std::string file);
    ziq_cfg getCfgFromFile(std::string file);
} // namespace zst_bb
#endif