#include "ziq2.h"
#ifdef BUILD_ZIQ2
#include "logger.h"
#include "dsp/buffer.h"
#include <volk/volk.h>

namespace ziq2
{
#ifdef _WIN32
#define __attribute__(x)
#pragma pack(push, 1)
#endif
    struct ziq2_info_pkt_hdr_t
    {
        uint64_t samplerate;
    } __attribute__((packed));
#ifdef _WIN32
#pragma pack(pop)
#endif

#ifdef _WIN32
#define __attribute__(x)
#pragma pack(push, 1)
#endif
    struct ziq2_iq_pkt_hdr_t
    {
        uint8_t bit_depth;
        float scale;
    } __attribute__((packed));
#ifdef _WIN32
#pragma pack(pop)
#endif

    int ziq2_write_info_pkt(uint8_t *output, uint64_t samplerate, bool sync)
    {
        if (sync)
        {
            output[0] = 0x1a;
            output[1] = 0xcf;
            output[2] = 0xfc;
            output[3] = 0x1d;
        }

        ziq2_pkt_hdr_t *mdr = (ziq2_pkt_hdr_t *)&output[sync ? 4 : 0];
        mdr->pkt_type = ZIQ2_PKT_INFO;

        ziq2_info_pkt_hdr_t *hdr = (ziq2_info_pkt_hdr_t *)&output[(sync ? 4 : 0) + sizeof(ziq2_pkt_hdr_t)];
        hdr->samplerate = samplerate;

        mdr->pkt_size = sizeof(ziq2_info_pkt_hdr_t);

        return (sync ? 4 : 0) + sizeof(ziq2_pkt_hdr_t) + mdr->pkt_size;
    }

    int ziq2_write_iq_pkt(uint8_t *output, complex_t *input, float *mag_buf, int nsamples, int bit_depth, bool sync)
    {
        if (sync)
        {
            output[0] = 0x1a;
            output[1] = 0xcf;
            output[2] = 0xfc;
            output[3] = 0x1d;
        }

        ziq2_pkt_hdr_t *mdr = (ziq2_pkt_hdr_t *)&output[sync ? 4 : 0];
        mdr->pkt_type = ZIQ2_PKT_IQ;

        ziq2_iq_pkt_hdr_t *hdr = (ziq2_iq_pkt_hdr_t *)&output[(sync ? 4 : 0) + sizeof(ziq2_pkt_hdr_t)];

        uint32_t max_index = 0;
        volk_32fc_magnitude_32f(mag_buf, (lv_32fc_t *)input, nsamples);
        volk_32f_index_max_32u(&max_index, mag_buf, nsamples);

        float scale = 0;

        if (bit_depth == 8)
            scale = 127.0f / mag_buf[max_index];
        else if (bit_depth == 16)
            scale = 65535.0f / mag_buf[max_index];

        hdr->bit_depth = bit_depth;
        hdr->scale = scale;

        int final_size = (sync ? 4 : 0) + sizeof(ziq2_pkt_hdr_t) + sizeof(ziq2_iq_pkt_hdr_t);

        if (bit_depth == 8)
        {
            volk_32f_s32f_convert_8i((int8_t *)&output[final_size], (float *)input, scale, nsamples * 2);
            final_size = nsamples * sizeof(int8_t) * 2;
        }
        else if (bit_depth == 16)
        {
            volk_32f_s32f_convert_16i((int16_t *)&output[final_size], (float *)input, scale, nsamples * 2);
            final_size = nsamples * sizeof(int16_t) * 2;
        }

        mdr->pkt_size = sizeof(ziq2_iq_pkt_hdr_t) + final_size;

        return (sync ? 4 : 0) + sizeof(ziq2_pkt_hdr_t) + mdr->pkt_size;
    }

    int ziq2_write_file_hdr(uint8_t *output, uint64_t samplerate, bool sync)
    {
        char *sig = (char *)&output[0];
        sig[0] = 'Z';
        sig[1] = 'I';
        sig[2] = 'Q';
        sig[3] = '2';

        return 4 + ziq2_write_info_pkt(&output[4], samplerate, sync);
    }

    void ziq2_read_info_pkt(uint8_t *input, uint64_t *samplerate)
    {
        ziq2_pkt_hdr_t *mdr = (ziq2_pkt_hdr_t *)&input[0];
        ziq2_info_pkt_hdr_t *hdr = (ziq2_info_pkt_hdr_t *)&input[sizeof(ziq2_pkt_hdr_t)];
        *samplerate = hdr->samplerate;
    }

    void ziq2_read_iq_pkt(uint8_t *input, complex_t *output, int *nsamples)
    {
        ziq2_pkt_hdr_t *mdr = (ziq2_pkt_hdr_t *)&input[0];
        ziq2_iq_pkt_hdr_t *hdr = (ziq2_iq_pkt_hdr_t *)&input[sizeof(ziq2_pkt_hdr_t)];
        *nsamples = ((mdr->pkt_size - sizeof(ziq2_iq_pkt_hdr_t)) * 8) / (hdr->bit_depth * 2);

        int final_size = sizeof(ziq2_pkt_hdr_t) + sizeof(ziq2_iq_pkt_hdr_t);

        if (hdr->bit_depth == 8)
            volk_8i_s32f_convert_32f((float *)output, (int8_t *)&input[final_size], hdr->scale, *nsamples * 2);
        else if (hdr->bit_depth == 16)
            volk_16i_s32f_convert_32f((float *)output, (int16_t *)&input[final_size], hdr->scale, *nsamples * 2);
    }

    bool ziq2_is_valid_ziq2(std::string file)
    {
        std::ifstream stream(file, std::ios::binary);
        char signature[4];
        stream.read((char *)signature, 4);
        stream.close();

        return std::string(&signature[0], &signature[4]) == ZIQ2_SIGNATURE;
    }

    uint64_t ziq2_try_parse_header(std::string file)
    {
        std::ifstream stream(file, std::ios::binary);
        stream.seekg(8);

        ziq2_pkt_hdr_t mdr;
        stream.read((char *)&mdr, sizeof(ziq2_pkt_hdr_t));

        if (mdr.pkt_type == ZIQ2_PKT_INFO)
        {
            ziq2_info_pkt_hdr_t hdr;
            stream.read((char *)&hdr, sizeof(ziq2_info_pkt_hdr_t));
            return hdr.samplerate;
        }

        return 0;
    }
};
#endif