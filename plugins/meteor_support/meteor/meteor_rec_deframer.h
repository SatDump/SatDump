#pragma once

#include <cstdint>

#define REC_FRM_SIZE 3072

namespace meteor
{
    class RecorderDeframer
    {
    private:
        uint8_t *shifting_buffer;

        bool synced = 0;
        int runs_since_sync = 0;

    public:
        RecorderDeframer()
        {
            shifting_buffer = new uint8_t[REC_FRM_SIZE];
        }

        ~RecorderDeframer()
        {
            delete[] shifting_buffer;
        }

        int work(uint8_t *input_bits, int length, uint8_t *output)
        {
            int nfrm = 0;

            for (int i = 0; i < length; i++)
            {
                memmove(&shifting_buffer[0], &shifting_buffer[1], REC_FRM_SIZE - 1);
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
                    uint8_t *bytes = &output[nfrm++ * (REC_FRM_SIZE / 8)];
                    for (int i = 0; i < REC_FRM_SIZE; i++)
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
}