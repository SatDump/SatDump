#pragma once

#include <cstring>
#include <cstdint>

#define MTVZA_REC_FRM_SIZE 3072
#define KMSS_BPSK_REC_FRM_SIZE 12288
#define KMSS_QPSK_REC_FRM_SIZE 24576

namespace meteor
{
    class MTVZA_ExtDeframer
    {
    private:
        uint8_t *shifting_buffer;

        bool synced = 0;
        int runs_since_sync = 0;

    public:
        MTVZA_ExtDeframer()
        {
            shifting_buffer = new uint8_t[MTVZA_REC_FRM_SIZE];
        }

        ~MTVZA_ExtDeframer()
        {
            delete[] shifting_buffer;
        }

        int work(uint8_t *input_bits, int length, uint8_t *output)
        {
            int nfrm = 0;

            for (int i = 0; i < length; i++)
            {
                memmove(&shifting_buffer[0], &shifting_buffer[1], MTVZA_REC_FRM_SIZE - 1);
                shifting_buffer[3072 - 1] = input_bits[i];

                bool sync1 = shifting_buffer[0] == 1 &&
                             shifting_buffer[1] == 0 &&
                             shifting_buffer[2] == 1 &&
                             shifting_buffer[3] == 1 &&
                             shifting_buffer[4] == 0 &&
                             shifting_buffer[5] == 0 &&
                             shifting_buffer[6] == 1 &&
                             shifting_buffer[7] == 1;

                bool sync2 = shifting_buffer[384 + 0] == 1 &&
                             shifting_buffer[384 + 1] == 1 &&
                             shifting_buffer[384 + 2] == 1 &&
                             shifting_buffer[384 + 3] == 0 &&
                             shifting_buffer[384 + 4] == 0 &&
                             shifting_buffer[384 + 5] == 0 &&
                             shifting_buffer[384 + 6] == 1 &&
                             shifting_buffer[384 + 7] == 1;

                bool sync3 = shifting_buffer[768 + 0] == 0 &&
                             shifting_buffer[768 + 1] == 1 &&
                             shifting_buffer[768 + 2] == 1 &&
                             shifting_buffer[768 + 3] == 1 &&
                             shifting_buffer[768 + 4] == 0 &&
                             shifting_buffer[768 + 5] == 1 &&
                             shifting_buffer[768 + 6] == 0 &&
                             shifting_buffer[768 + 7] == 1;

                bool sync4 = shifting_buffer[1920 + 0] == 0 &&
                             shifting_buffer[1920 + 1] == 0 &&
                             shifting_buffer[1920 + 2] == 0 &&
                             shifting_buffer[1920 + 3] == 0 &&
                             shifting_buffer[1920 + 4] == 0 &&
                             shifting_buffer[1920 + 5] == 0 &&
                             shifting_buffer[1920 + 6] == 0 &&
                             shifting_buffer[1920 + 8] == 0;

                runs_since_sync++;

                if (synced ? (sync1 && sync2 && sync3 && sync4) : ((int)sync1 + (int)sync2 + (int)sync3 + (int)sync4) > 2)
                {
                    uint8_t *bytes = &output[nfrm++ * (MTVZA_REC_FRM_SIZE / 8)];
                    for (int i = 0; i < MTVZA_REC_FRM_SIZE; i++)
                        bytes[i / 8] = bytes[i / 8] << 1 | shifting_buffer[i];

                    if (runs_since_sync > 2 && !synced)
                    {
                        synced = true;
                        runs_since_sync = 0;
                        // logger->critical("SYNC!");
                    }
                }
                else
                {
                    runs_since_sync++;
                    synced = false;
                }
            }

            return nfrm;
        }

        bool getSynced()
        {
            return synced;
        }
    };

    class KMSS_BPSK_ExtDeframer
    {
    private:
        uint8_t *shift_buffer;

        bool synced = 0;
        int runs_since_sync = 0;
        int skip_bits = 0;

        uint8_t sync_reg_1[2];
        uint8_t sync_reg_2[4];
        // uint8_t sync_reg_3[4];
        // uint8_t sync_reg_4[4];
        // uint8_t sync_reg_5[4];
        // uint8_t sync_reg_6[4];
        // uint8_t sync_reg_7[4];

    private:
        int compare_8(uint8_t v1, uint8_t v2)
        {
            int cor = 0;
            uint8_t diff = v1 ^ v2;
            for (; diff; cor++)
                diff &= diff - 1;
            return cor;
        }

        uint8_t repack_8(uint8_t *bit)
        {

            return bit[0] << 7 |
                   bit[1] << 6 |
                   bit[2] << 5 |
                   bit[3] << 4 |
                   bit[4] << 3 |
                   bit[5] << 2 |
                   bit[6] << 1 |
                   bit[7];
        }

    public:
        KMSS_BPSK_ExtDeframer()
        {
            shift_buffer = new uint8_t[KMSS_BPSK_REC_FRM_SIZE];
        }

        ~KMSS_BPSK_ExtDeframer()
        {
            delete[] shift_buffer;
        }

        int work(uint8_t *input_bytes, int length, uint8_t *output)
        {
            int frames_out = 0;
            for (int i = 0; i < length; i++)
            {
                memmove(&shift_buffer[0], &shift_buffer[1], ((KMSS_BPSK_REC_FRM_SIZE / 8) + 1) - 1);
                shift_buffer[((KMSS_BPSK_REC_FRM_SIZE / 8) + 1) - 1] = input_bytes[i];

                for (int s = 0; s < 8; s++)
                {
                    if (skip_bits-- > 0)
                        continue;

                    sync_reg_1[0] = ((shift_buffer[0 + 0] << s) | (shift_buffer[0 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_1[1] = ((shift_buffer[1 + 0] << s) | (shift_buffer[1 + 1] >> (8 - s))) & 0xFF;

                    sync_reg_2[0] = ((shift_buffer[382 + 0] << s) | (shift_buffer[382 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_2[1] = ((shift_buffer[383 + 0] << s) | (shift_buffer[383 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_2[2] = ((shift_buffer[384 + 0] << s) | (shift_buffer[384 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_2[3] = ((shift_buffer[385 + 0] << s) | (shift_buffer[385 + 1] >> (8 - s))) & 0xFF;

#if 0
                           sync_reg_3[0] = ((shift_buffer[574 + 0] << s) | (shift_buffer[574 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_3[1] = ((shift_buffer[575 + 0] << s) | (shift_buffer[575 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_3[2] = ((shift_buffer[576 + 0] << s) | (shift_buffer[576 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_3[3] = ((shift_buffer[577 + 0] << s) | (shift_buffer[577 + 1] >> (8 - s))) & 0xFF;

                    sync_reg_4[0] = ((shift_buffer[766 + 0] << s) | (shift_buffer[766 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_4[1] = ((shift_buffer[767 + 0] << s) | (shift_buffer[767 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_4[2] = ((shift_buffer[768 + 0] << s) | (shift_buffer[768 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_4[3] = ((shift_buffer[769 + 0] << s) | (shift_buffer[769 + 1] >> (8 - s))) & 0xFF;

                    sync_reg_5[0] = ((shift_buffer[958 + 0] << s) | (shift_buffer[958 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_5[1] = ((shift_buffer[959 + 0] << s) | (shift_buffer[959 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_5[2] = ((shift_buffer[960 + 0] << s) | (shift_buffer[960 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_5[3] = ((shift_buffer[961 + 0] << s) | (shift_buffer[961 + 1] >> (8 - s))) & 0xFF;

                    sync_reg_6[0] = ((shift_buffer[1150 + 0] << s) | (shift_buffer[1150 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_6[1] = ((shift_buffer[1151 + 0] << s) | (shift_buffer[1151 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_6[2] = ((shift_buffer[1152 + 0] << s) | (shift_buffer[1152 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_6[3] = ((shift_buffer[1153 + 0] << s) | (shift_buffer[1153 + 1] >> (8 - s))) & 0xFF;

                    sync_reg_7[0] = ((shift_buffer[1342 + 0] << s) | (shift_buffer[1342 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_7[1] = ((shift_buffer[1343 + 0] << s) | (shift_buffer[1343 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_7[2] = ((shift_buffer[1344 + 0] << s) | (shift_buffer[1344 + 1] >> (8 - s))) & 0xFF;
                    sync_reg_7[3] = ((shift_buffer[1345 + 0] << s) | (shift_buffer[1345 + 1] >> (8 - s))) & 0xFF;
#endif

                    int sync1 = compare_8(sync_reg_1[0], 0x00) +
                                compare_8(sync_reg_1[1], 0x35);
                    int sync2 = compare_8(sync_reg_2[0], 0x00) +
                                compare_8(sync_reg_2[1], 0x00) +
                                compare_8(sync_reg_2[2], 0x00) +
                                compare_8(sync_reg_2[3], 0x00);

#if 0
                    int sync3 = compare_8(sync_reg_3[0], 0x0f) +
                                compare_8(sync_reg_3[1], 0x0f) +
                                compare_8(sync_reg_3[2], 0x0f) +
                                compare_8(sync_reg_3[3], 0x0f);

                    int sync4 = compare_8(sync_reg_4[0], 0x00) +
                                compare_8(sync_reg_4[1], 0x00) +
                                compare_8(sync_reg_4[2], 0x0f) +
                                compare_8(sync_reg_4[3], 0x00);

                    int sync5 = compare_8(sync_reg_5[0], 0xf0) +
                                compare_8(sync_reg_5[1], 0xff) +
                                compare_8(sync_reg_5[2], 0x00) +
                                compare_8(sync_reg_5[3], 0xff);

                    int sync6 = compare_8(sync_reg_6[0], 0xff) +
                                compare_8(sync_reg_6[1], 0xf0) +
                                compare_8(sync_reg_6[2], 0x00) +
                                compare_8(sync_reg_6[3], 0xff);

                    int sync7 = compare_8(sync_reg_7[0], 0x0f) +
                                compare_8(sync_reg_7[1], 0xff) +
                                compare_8(sync_reg_7[2], 0x0f) +
                                compare_8(sync_reg_7[3], 0x0f);

                    if (synced
                            ? (sync1 < 10 && sync2 < 20 && sync3 < 20 && sync4 < 20 && sync5 < 20 && sync6 < 20 && sync7 < 20)
                            : (sync1 < 6 && sync2 < 10 && sync3 < 10 && sync4 < 10 && sync5 < 10 && sync6 < 10 && sync7 < 10))
#endif

                    if (synced ? (sync1 < 6 && sync2 < 8) : (sync1 < 1 && sync2 < 3))
                    {
                        for (int ii = 0; ii < (KMSS_BPSK_REC_FRM_SIZE / 8); ii++)
                            output[frames_out * (KMSS_BPSK_REC_FRM_SIZE / 8) + ii] = ((shift_buffer[ii + 0] << s) | (shift_buffer[ii + 1] >> (8 - s))) & 0xFF;
                        frames_out++;

                        synced = true;
                        skip_bits = KMSS_BPSK_REC_FRM_SIZE - 1;
                    }
                    else
                    {
                        synced = false;
                    }
                }
            }
            return frames_out;
        }
    };

    class KMSS_QPSK_ExtDeframer
    {
    private:
        uint8_t *shift_buffer;

        bool synced = 0;
        int runs_since_sync = 0;

        int skip_bits = 0;

        uint8_t sync_reg_1[4];
        uint8_t sync_reg_2[8];

    private:
        int compare_8(uint8_t v1, uint8_t v2);
        uint8_t repack_8(uint8_t *bit);

    public:
        KMSS_QPSK_ExtDeframer();
        ~KMSS_QPSK_ExtDeframer();

        int work(uint8_t *input_bytes, int length, uint8_t *output);
    };
}