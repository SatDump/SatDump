/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "common/simple_deframer.h"
#include "handlers/experimental/decoupled/rec_backend.h"
#include "handlers/experimental/decoupled/rec_frontend.h"
#include "handlers/experimental/decoupled/test/test_http.h"
#include "image/image.h"
#include "image/io.h"
#include "init.h"
#include "logger.h"
#include "utils/binary.h"
#include "utils/time.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>

extern "C"
{
#include "wip/gfsk.h"
}

namespace
{
    inline void bitcpy(void *v_dst, const void *v_src, size_t offset, size_t bits)
    {
        uint8_t *dst = (uint8_t *)v_dst;
        const uint8_t *src = (uint8_t *)v_src;

        src += offset / 8;
        offset %= 8;

        /* All but last reads */
        for (; bits > 8; bits -= 8)
        {
            *dst = *src++ << offset;
            *dst++ |= *src >> (8 - offset);
        }

        /* Last read */
        if (offset + bits < 8)
        {
            *dst = (*src << offset) & ~((1 << (8 - bits)) - 1);
        }
        else
        {
            *dst = *src++ << offset;
            *dst |= *src >> (8 - offset);
            *dst &= ~((1 << (8 - bits)) - 1);
        }
    }

    inline void manchester_decode(void *dst, const void *src, int nbits)
    {
        uint8_t *raw_dst = (uint8_t *)dst;
        uint8_t out;
        uint8_t inBits;
        int i, out_count;

        out = 0;
        out_count = 0;
        for (i = 0; i < nbits; i += 2)
        {
            bitcpy(&inBits, src, i, 2);
            out = (out << 1) | (inBits & 0x40 ? 1 : 0);
            out_count++;

            if (!(out_count % 8))
            {
                *raw_dst++ = out;
                out = 0;
            }
        }
        *raw_dst = out;
    }

    void m10_frame_descramble(uint8_t *frame)
    {
        uint8_t *raw_frame = (uint8_t *)frame;
        uint8_t tmp, topbit;
        int i;

        topbit = 0;
        for (i = 0; i < 104; i++)
        {
            tmp = raw_frame[i] << 7;
            raw_frame[i] ^= 0xFF ^ (topbit | raw_frame[i] >> 1);
            topbit = tmp;
        }
    }

#define M10_MAX_DATA_LEN 99

#pragma pack(push, 1)
    typedef struct
    {
        uint8_t sync_mark[3];
        uint8_t len;
        uint8_t type;
        uint8_t data[M10_MAX_DATA_LEN];
    } M10Frame;
#pragma pack(pop)

    uint16_t m10_crc_step(uint16_t c, uint8_t b)
    {
        int c0, c1, t, t6, t7, s;
        c1 = c & 0xFF;
        // B
        b = (b >> 1) | ((b & 1) << 7);
        b ^= (b >> 2) & 0xFF;
        // A1
        t6 = (c & 1) ^ ((c >> 2) & 1) ^ ((c >> 4) & 1);
        t7 = ((c >> 1) & 1) ^ ((c >> 3) & 1) ^ ((c >> 5) & 1);
        t = (c & 0x3F) | (t6 << 6) | (t7 << 7);
        // A2
        s = (c >> 7) & 0xFF;
        s ^= (s >> 2) & 0xFF;
        c0 = b ^ t ^ s;
        return ((c1 << 8) | c0) & 0xFFFF;
    }

    int m10_frame_correct(M10Frame *frame)
    {
        const uint8_t *raw_frame = (uint8_t *)&frame->len;
        const uint8_t *crc_ptr = (uint8_t *)&frame->len + frame->len - 1;
        const uint16_t expected = crc_ptr[0] << 8 | crc_ptr[1];
        uint16_t crc;

        crc = 0;
        for (; raw_frame < crc_ptr; raw_frame++)
        {
            crc = m10_crc_step(crc, *raw_frame);
        }

        return crc == expected;
    }

#pragma pack(push, 1)
    /* Specific subframe types {{{ */
    typedef struct
    {
        uint8_t sync_mark[3];
        uint8_t len;
        uint8_t type; /* 0x9f */

        uint8_t small_values[2];
        uint8_t dlat[2]; /* x velocity */
        uint8_t dlon[2]; /* y velocity */
        uint8_t dalt[2]; /* z velocity */
        uint8_t time[4]; /* GPS time */
        uint8_t lat[4];
        uint8_t lon[4];
        uint8_t alt[4];
        uint8_t _pad0[4];
        uint8_t sat_count; /* Number of satellites used for fix */
        uint8_t _pad3;
        uint8_t week[2]; /* GPS week */

        uint8_t prn[12]; /* PRNs of satellites used for fix */
        uint8_t _pad1[4];
        uint8_t rh_ref[3];    /* RH reading @ 55% */
        uint8_t rh_counts[3]; /* RH reading */
        uint8_t _pad2[6];
        uint8_t adc_temp_range;  /* Temperature range index */
        uint8_t adc_temp_val[2]; /* Temperature ADC value */
        uint8_t unk0[4];         /* Probably related to temp range */
        uint8_t adc_batt_val[2];
        uint8_t unk3[2]; /* Correlated to adc_battery_val, also very linear */
        uint8_t _pad4[12];
        uint8_t unk4[2]; /* Fairly constant */
        uint8_t unk5[2]; /* Fairly constant */
        uint8_t unk6[2]; /* Correlated to unk0 */
        uint8_t unk7[2];

        uint8_t serial[5];
        uint8_t seq;
    } M10Frame_9f;
#pragma pack(pop)

#define GPS_EPOCH_DELTA 315964800UL
#define SECONDS_PER_WEEK (86400UL * 7)

    time_t gps_time_to_utc(uint16_t week, uint32_t ms) { return (time_t)(ms / 1000UL) + (SECONDS_PER_WEEK * week) + GPS_EPOCH_DELTA; }

    time_t m10_9f_time(const M10Frame_9f *f)
    {
        const uint32_t ms = f->time[0] << 24 | f->time[1] << 16 | f->time[2] << 8 | f->time[3];
        const uint16_t week = f->week[0] << 8 | f->week[1];

        return gps_time_to_utc(week, ms);
    }
} // namespace

int main(int argc, char *argv[])
{
    initLogger();
    logger->set_level(slog::LOG_ERROR);
    satdump::initSatDump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    std::ifstream dat(argv[1]);

    float last_val = 0, val[8192];

    int last_pos = 0, pos = 0;

    int sample_offset = 0;

    // 0x66666666b366
    std::ofstream out("/tmp/test.bin");

    auto def = std::make_shared<def::SimpleDeframer>(0x66666666b366, 48, 1664, 1, false, false);

    uint8_t bit_shifter;
    int in_bit_shifter = 0;

    // cr_reset();

#if 0
    while (!dat.eof())
    {
        dat.read((char *)&bit_shifter, 1);

        auto frms = def->work((uint8_t *)&bit_shifter, 1);

        for (auto &frm : frms)
        {
            // printf("FRAME\n");

            uint8_t obuf[208];

            memcpy(obuf, frm.data(), 208);

            manchester_decode(obuf, frm.data(), 208 * 8);
            // memcpy(obuf, frm.data(), 208);
            m10_frame_descramble(obuf);

            if (m10_frame_correct((M10Frame *)obuf))
            {
                out.write((char *)obuf, 104);

                if (((M10Frame *)obuf)->type == 0x9F)
                {
                    printf("TIME %s\n", satdump::timestamp_to_string(m10_9f_time((M10Frame_9f *)obuf)).c_str());
                }
            }
        }

        in_bit_shifter = 0;

#else
    GFSKDemod demod;
    gfsk_init(&demod, 48000, 9600);
    size_t bit_offset = 0;
    float outbits[8192];

    uint8_t shift_buf[1664];

    auto sync = satdump::unsigned_to_bitvec<uint64_t>(0x66666666b3660000);

    sync.resize(48);

    while (!dat.eof())
    {
        dat.read((char *)&val, 8192 * sizeof(float));

        bit_offset = 0;
        gfsk_demod(&demod, outbits, &bit_offset, 8192, val, 8192);

#if 1
        for (int ib = 0; ib < bit_offset; ib++)
        {
            memmove(shift_buf, shift_buf + 1, 1664 - 1);
            shift_buf[1663] = outbits[ib] > 0;

            int best_corr = 0;
            int best_corr_pos = -1;
            for (int ii = 0; ii < 1664 - 48; ii++)
            {
                int corr = 0;
                for (int i = 0; i < 48; i++)
                    if (shift_buf[ii + i] == sync[ii + i])
                        corr++;

                if (best_corr < corr)
                {
                    best_corr = corr;
                    best_corr_pos = ii;
                }
            }

            if (best_corr_pos == 0)
            {
                uint8_t obuf2[208];
                uint8_t obuf[208];

                for (int i = 0; i < 1664; i++)
                    obuf2[i / 8] = obuf2[i / 8] << 1 | shift_buf[i];

                manchester_decode(obuf, obuf2, 208 * 8);
                // memcpy(obuf, frm.data(), 208);
                m10_frame_descramble(obuf);

                if (((M10Frame *)obuf)->len == 0)
                    continue;

                if (m10_frame_correct((M10Frame *)obuf))
                {

                    if (((M10Frame *)obuf)->type == 0x9F)
                    {
                        printf("TIME %s\n", satdump::timestamp_to_string(m10_9f_time((M10Frame_9f *)obuf)).c_str());

                        out.write((char *)obuf, 104);
                        out.flush();
                        // printf("FRAME\n");
                    }
                }
            }
        }
#elif 0
        for (int ib = 0; ib < bit_offset; ib++)
        {
            bit_shifter = bit_shifter << 1 | (outbits[ib] > 0);
            in_bit_shifter++;

            if (in_bit_shifter >= 8)
            {
                auto frms = def->work((uint8_t *)&bit_shifter, 1);

                for (auto &frm : frms)
                {
                    // printf("FRAME\n");

                    uint8_t obuf[208];

                    memcpy(obuf, frm.data(), 208);

                    manchester_decode(obuf, frm.data(), 208 * 8);
                    // memcpy(obuf, frm.data(), 208);
                    m10_frame_descramble(obuf);

                    if (m10_frame_correct((M10Frame *)obuf))
                    {
                        out.write((char *)obuf, 104);

                        if (((M10Frame *)obuf)->type == 0x9F)
                        {
                            printf("TIME %s\n", satdump::timestamp_to_string(m10_9f_time((M10Frame_9f *)obuf)).c_str());
                        }
                    }
                }

                in_bit_shifter = 0;
            }
        }
#endif
#endif

#if 0
        if (clock_recovery(val, &last_val))
        {
            int8_t vvv = last_val > 0 ? 70 : -70;
            // out.write((char *)&vvv, 1);

            bit_shifter = bit_shifter << 1 | (last_val > 0);
            in_bit_shifter++;

            if (in_bit_shifter >= 8)
            {
                auto frms = def->work((uint8_t *)&bit_shifter, 1);

                for (auto &frm : frms)
                {
                    // printf("FRAME\n");

                    uint8_t obuf[208];

                    memcpy(obuf, frm.data(), 208);

                    manchester_decode(obuf, frm.data(), 208 * 8);
                    // memcpy(obuf, frm.data(), 208);
                    m10_frame_descramble(obuf);

                    if (m10_frame_correct((M10Frame *)obuf))
                    {
                        out.write((char *)obuf, 104);

                        if (((M10Frame *)obuf)->type == 0x9F)
                        {
                            printf("TIME %s\n", satdump::timestamp_to_string(m10_9f_time((M10Frame_9f *)obuf)).c_str());
                        }
                    }
                }

                in_bit_shifter = 0;
            }
        }
#endif

#if 0
        if ((val > 0 && last_val < 0) || (last_val > 0 && val < 0))
        {
            printf("Zero Crossing %d\n", pos - last_pos);
            last_pos = pos;
            sample_offset = 0;
        }

        if (sample_offset++ % 4 == 3)
        {
            // Get output
            int8_t vvv = val > 0 ? 70 : -70;
            out.write((char *)&vvv, 1);
        }

        pos++;

        last_val = val;
#endif
    }
}