#include "clock_recovery_gardner2.h"
#include <algorithm>

#define MIN_SAMPLES_PER_SYMBOL 8
#define FREQ_DEV_EXP 8

namespace dsp
{
    float rc_coeff(float cutoff, int stage_no, unsigned taps, float osf, float alpha);

    GardnerClockRecovery2Block::GardnerClockRecovery2Block(std::shared_ptr<dsp::stream<float>> input,
                                                           float samplerate, float symbolrate,
                                                           float zeta, int order)
        : Block<float, float>(input)
    {
        const float sym_freq = (float)symbolrate / samplerate;
        const int num_phases = 1 + (MIN_SAMPLES_PER_SYMBOL * sym_freq);

        // LPF init
        {
            float cutoff = 3 * sym_freq;
            const int taps = order * 2 + 1;

            lpf_coeffs = (float *)malloc(num_phases * sizeof(float) * taps);
            lpf_mem = (float *)calloc(2 * taps, sizeof(float));

            cutoff /= num_phases;

            for (int phase = 0; phase < num_phases; phase++)
                for (int i = 0; i < taps; i++)
                    lpf_coeffs[phase * taps + i] = rc_coeff(cutoff, i * num_phases + phase, taps * num_phases, num_phases, 0.99);

            lpf_num_phases = num_phases;
            lpf_size = taps;
            lpf_idx = 0;
        }

        // Tim Init
        {
            const float bw = sym_freq / num_phases / 100.0;

            tim_freq = 2 * sym_freq;
            tim_center_freq = 2 * sym_freq;
            tim_max_fdev = sym_freq / (1 << FREQ_DEV_EXP);

            tim_phase = 0;
            tim_state = 1;
            tim_prev = 0;

            float damp = zeta;
            float denom = (1 + 2 * damp * bw + bw * bw);
            tim_alpha = 4 * damp * bw / denom;
            tim_beta = 4 * bw * bw / denom;
        }
    }

    GardnerClockRecovery2Block::~GardnerClockRecovery2Block()
    {
        free(lpf_coeffs);
        free(lpf_mem);
    }

    void GardnerClockRecovery2Block::work()
    {
        int nsamples = Block<float, float>::input_stream->read();
        if (nsamples <= 0)
        {
            Block<float, float>::input_stream->flush();
            return;
        }

        int outc = 0;
        interm = 0;

        for (int isample = 0; isample < nsamples; isample++)
        {
            float symbol = Block<float, float>::input_stream->readBuf[isample];

            // Feed interpolator
            lpf_mem[lpf_idx] = lpf_mem[lpf_idx + lpf_size] = symbol;
            lpf_idx = (lpf_idx + 1) % lpf_size;

            // Recover symbol value
            for (int phase = 0; phase < lpf_num_phases; phase++)
            {
                int timeslot = 0;

                // Advance timeslot
                tim_phase += tim_freq;

                // Check if the timeslot is right
                if (tim_phase >= tim_state)
                {
                    timeslot = tim_state;
                    tim_state = (tim_state % 2) + 1;
                }

                // Process symbol
                switch (timeslot)
                {
                case 1:
                    // Half-way slot
                    volk_32f_x2_dot_prod_32f(&interm, &lpf_mem[lpf_idx], &lpf_coeffs[lpf_size * (lpf_num_phases - phase - 1)], lpf_size);
                    break;
                case 2:
                    // Correct slot: update time estimate
                    volk_32f_x2_dot_prod_32f(&symbol, &lpf_mem[lpf_idx], &lpf_coeffs[lpf_size * (lpf_num_phases - phase - 1)], lpf_size);

                    {
                        // Compute timing error, Gardner
                        float error = (symbol * tim_prev < 0) ? (symbol - tim_prev) * interm : 0;
                        tim_prev = symbol;

                        // Update phase and freq estimate
                        float freq_delta = tim_phase - tim_center_freq;

                        tim_phase -= 2 - std::max<float>(-2, std::min<float>(2, error * tim_alpha));
                        freq_delta += error * tim_beta;

                        freq_delta = std::max<float>(-tim_max_fdev, std::min<float>(tim_max_fdev, freq_delta));
                        tim_freq = tim_center_freq + freq_delta;
                    }

                    // Write output
                    Block<float, float>::output_stream->writeBuf[outc++] = symbol;

                    break;
                default:
                    break;
                }
            }
        }

        Block<float, float>::input_stream->flush();
        Block<float, float>::output_stream->swap(outc);
    }

    float rc_coeff(float cutoff, int stage_no, unsigned taps, float osf, float alpha)
    {
        float rc_coeff = 0, hamming_coeff = 0;
        const int order = (taps - 1) / 2;
        const float norm = 2.0 / 5.0 * order / osf / 10.0;

        float t = abs(order - stage_no) / osf;

        if (t == 0)
            rc_coeff = cutoff;
        else if (2 * alpha * t * cutoff == 1)
            rc_coeff = -M_PI / (4 * cutoff) * sinf(M_PI / (2 * alpha)) / (M_PI / (2 * alpha));
        else // Raised cosine coefficient
            rc_coeff = sinf(M_PI * t * cutoff) / (M_PI * t) * cosf(M_PI * alpha * t * cutoff) / (1 - powf(2 * alpha * t * cutoff, 2));

        // Hamming windowing function
        hamming_coeff = 0.42 - 0.5 * cosf(2 * M_PI * stage_no / (taps - 1)) + 0.08 * cosf(4 * M_PI * stage_no / (taps - 1));

        return norm * rc_coeff * hamming_coeff;
    }
}