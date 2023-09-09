#pragma once

#include <cstdint>
#include "common/dsp/buffer.h"
#include <volk/volk.h>
#include "common/dsp/complex.h"

// Copied from ZIQ2 for now... Expected to change...
namespace remote_sdr
{
    inline int encode_iq_pkt(uint8_t *output, complex_t *input, float *mag_buf, int nsamples, int bit_depth)
    {
        uint32_t max_index = 0;
        volk_32fc_magnitude_32f(mag_buf, (lv_32fc_t *)input, nsamples);
        volk_32f_index_max_32u(&max_index, mag_buf, nsamples);

        float scale = 0;

        if (bit_depth == 8)
            scale = 127.0f / mag_buf[max_index];
        else if (bit_depth == 16)
            scale = 65535.0f / mag_buf[max_index];

        output[0] = bit_depth;
        *((float *)&output[1]) = scale;
        *((uint32_t *)&output[5]) = nsamples;

        int final_size = 9;

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

        return final_size;
    }

    inline void decode_iq_pkt(uint8_t *input, complex_t *output, int *nsamples)
    {
        int bit_depth = input[0];
        float scale = *((float *)&input[1]);
        *nsamples = *((uint32_t *)&input[5]);

        int final_size = 9;

        if (bit_depth == 8)
            volk_8i_s32f_convert_32f((float *)output, (int8_t *)&input[final_size], scale, *nsamples * 2);
        else if (bit_depth == 16)
            volk_16i_s32f_convert_32f((float *)output, (int16_t *)&input[final_size], scale, *nsamples * 2);
    }
};