#include "generic_correlator.h"
#include "common/dsp/buffer.h"
#include "rotation.h"

CorrelatorGeneric::CorrelatorGeneric(dsp::constellation_type_t mod, std::vector<uint8_t> syncword, int max_frm_size) : d_modulation(mod)
{
    converted_buffer = dsp::create_volk_buffer<float>(max_frm_size * 2);
    syncword_length = syncword.size();

    // Generate syncwords
    if (d_modulation == dsp::BPSK)
    {
        syncwords.resize(2);
        for (int i = 0; i < 2; i++)
        {
            syncwords[i].resize(syncword_length);
            modulate_soft(syncwords[i].data(), syncword.data(), syncword_length);
        }
        rotate_float_buf(syncwords[1].data(), syncword_length, 180);
    }
    else if (d_modulation == dsp::QPSK)
    {
        syncwords.resize(4);
        for (int i = 0; i < 4; i++)
        {
            syncwords[i].resize(syncword_length);
            modulate_soft(syncwords[i].data(), syncword.data(), syncword_length);
        }
        rotate_float_buf(syncwords[1].data(), syncword_length, 90);
        rotate_float_buf(syncwords[2].data(), syncword_length, 180);
        rotate_float_buf(syncwords[3].data(), syncword_length, 270);
    }
    else if (d_modulation == dsp::OQPSK)
    {
        syncwords.resize(8);

        for (int i = 0; i < 4; i++)
        {
            syncwords[i].resize(syncword_length);
            modulate_soft(syncwords[i].data(), syncword.data(), syncword_length);
        }

        // Syncword if we delayed the wrong phase
        uint8_t last_q_oqpsk = 0;
        for (int i = 0; i < syncword_length / 2; i++)
        {
            uint8_t back = syncword[i * 2 + 1];
            syncword[i * 2 + 1] = last_q_oqpsk;
            last_q_oqpsk = back;
        }

        for (int i = 4; i < 8; i++)
        {
            syncwords[i].resize(syncword_length);
            modulate_soft(syncwords[i].data(), syncword.data(), syncword_length);
        }

        rotate_float_buf(syncwords[1].data(), syncword_length, 90);
        rotate_float_buf(syncwords[2].data(), syncword_length, 180);
        rotate_float_buf(syncwords[3].data(), syncword_length, 270);

        rotate_float_buf(syncwords[5].data(), syncword_length, 90);
        rotate_float_buf(syncwords[6].data(), syncword_length, 180);
        rotate_float_buf(syncwords[7].data(), syncword_length, 270);
    }
}

void CorrelatorGeneric::rotate_float_buf(float *buf, int size, float rot_deg)
{
    float phase = rot_deg * 0.01745329;
    complex_t *cc = (complex_t *)buf;
    for (int i = 0; i < size / 2; i++)
        cc[i] = cc[i] * complex_t(cosf(phase), sinf(phase));
}

void CorrelatorGeneric::modulate_soft(float *buf, uint8_t *bit, int size)
{
    for (int i = 0; i < size; i++)
        buf[i] = bit[i] ? 1.0f : -1.0f;
}

CorrelatorGeneric::~CorrelatorGeneric()
{
    volk_free(converted_buffer);
}

int CorrelatorGeneric::correlate(int8_t *soft_input, phase_t &phase, bool &swap, float &cor, int length)
{
    volk_8i_s32f_convert_32f(converted_buffer, soft_input, 127 / 2, length);

    float corr = 0;
    int position = 0, best_sync = 0;
    cor = 0;

    for (int i = 0; i < length - syncword_length; i++)
    {
        for (int s = 0; s < syncwords.size(); s++)
        {
            volk_32f_x2_dot_prod_32f(&corr, &converted_buffer[i], syncwords[s].data(), syncword_length);

            if (corr > cor)
            {
                cor = corr;
                best_sync = s;
                position = i;
            }
        }
    }

    if (d_modulation == dsp::BPSK)
    {
        phase = best_sync ? PHASE_180 : PHASE_0;
        swap = false;
    }
    else if (d_modulation == dsp::QPSK || d_modulation == dsp::OQPSK)
    {
        phase = (phase_t)(best_sync % 4);
        swap = (best_sync / 4);
    }

    return position;
}