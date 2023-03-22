#pragma once

#ifdef BUILD_ZIQ2
#include <string>
#include <cstdint>
#include <fstream>
#include <zstd.h>
#include "dsp/complex.h"

// File signature of a ZIQ2 File
#define ZIQ2_SIGNATURE "ZIQ2"

namespace ziq2
{
    enum ziq2_pkt_type_t
    {
        ZIQ2_PKT_INFO = 0,
        ZIQ2_PKT_IQ = 1,
    };

#ifdef _WIN32
#define __attribute__(x)
#pragma pack(push, 1)
#endif
    struct ziq2_pkt_hdr_t
    {
        uint32_t pkt_size;
        uint8_t pkt_type;
    } __attribute__((packed));
#ifdef _WIN32
#pragma pack(pop)
#endif

    int ziq2_write_info_pkt(uint8_t *output, uint64_t samplerate, bool sync = true);
    int ziq2_write_iq_pkt(uint8_t *output, complex_t *input, float *mag_buf, int nsamples, int bit_depth, bool sync = true);
    int ziq2_write_file_hdr(uint8_t *output, uint64_t samplerate, bool sync = true);

    void ziq2_read_info_pkt(uint8_t *input, uint64_t *samplerate);
    void ziq2_read_iq_pkt(uint8_t *input, complex_t *output, int *nsamples);

    bool ziq2_is_valid_ziq2(std::string file);
    uint64_t ziq2_try_parse_header(std::string file);
}
#endif