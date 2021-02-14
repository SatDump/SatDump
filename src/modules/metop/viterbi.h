#include <complex>
#include <vector>

#define TestBitsLen 1024

extern "C"
{
#include "viterbi_lib/viterbi.h"
}

class MetopViterbi
{
private:
    bool d_sync_check;
    float d_ber_threshold;
    int d_insync_after;
    int d_outsync_after;
    int d_reset_after;

    float ber_calc1(struct viterbi_state *state0, struct viterbi_state *state1, unsigned int symsnr, unsigned char *insymbols_I, unsigned char *insymbols_Q);
    void phase_move_two(unsigned char phase_state, unsigned int symsnr, unsigned char *in_I, unsigned char *in_Q, unsigned char *out_I, unsigned char *out_Q);

    void do_reset();
    void enter_idle();
    void enter_synced();

    int d_mettab[2][256]; //metric table for continuous decoder

    struct viterbi_state d_state0[64]; //main viterbi decoder state 0 memory
    struct viterbi_state d_state1[64]; //main viterbi decoder state 1 memory

    unsigned char d_viterbi_in[4];
    bool d_viterbi_enable;
    unsigned int d_shift, d_shift_main_decoder;
    unsigned int d_phase;
    bool d_valid_ber_found;

    unsigned char d_bits, d_sym_count;
    bool d_even_symbol_last, d_curr_is_even, d_even_symbol;

    struct viterbi_state d_00_st0[64];
    struct viterbi_state d_00_st1[64];

    struct viterbi_state d_180_st0[64];
    struct viterbi_state d_180_st1[64];

    float d_ber[4][8];
    unsigned int d_chan_len;            //BER test input syms
    unsigned char d_valid_packet_count; //count packets with valid BER
    unsigned char d_invalid_packet_count;
    unsigned char d_state;

public:
    MetopViterbi(bool sync_check, float ber_threshold, int insync_after, int outsync_after, int reset_after);
    ~MetopViterbi();

    unsigned char &getState();

    int work(std::complex<float> *in, size_t size, uint8_t *output);
};