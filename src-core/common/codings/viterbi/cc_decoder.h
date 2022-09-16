#pragma once

#include "cc_common.h"
#include <volk/volk_alloc.hh>
#include <map>

namespace viterbi
{
    typedef void (*conv_kernel)(unsigned char *Y,
                                unsigned char *X,
                                unsigned char *syms,
                                unsigned char *dec,
                                unsigned int framebits,
                                unsigned int excess,
                                unsigned char *Branchtab);

    class CCDecoder
    {
    private:
        // everything else...
        void create_viterbi();
        int init_viterbi(struct v *vp, int starting_state);
        int init_viterbi_unbiased(struct v *vp);
        int update_viterbi_blk(unsigned char *syms, int nbits);
        int chainback_viterbi(unsigned char *data,
                              unsigned int nbits,
                              unsigned int endstate,
                              unsigned int tailsize);
        int find_endstate();

        volk::vector<unsigned char> d_branchtab;
        unsigned char Partab[256];

        int d_ADDSHIFT;
        int d_SUBSHIFT;
        conv_kernel d_kernel;
        unsigned int d_max_frame_size;
        unsigned int d_frame_size;
        unsigned int d_k;
        unsigned int d_rate;
        std::vector<int> d_polys;

        struct v d_vp;
        volk::vector<unsigned char> d_managed_in;
        int d_numstates;
        int d_decision_t_size;
        int *d_start_state;
        int d_start_state_chaining;
        int d_start_state_nonchaining;
        int *d_end_state;
        int d_end_state_chaining;
        int d_end_state_nonchaining;
        unsigned int d_veclen;

        int parity(int x);
        int parityb(unsigned char x);
        void partab_init(void);

    public:
        CCDecoder(int frame_size,
                  int k,
                  int rate,
                  std::vector<int> polys,
                  int start_state = 0,
                  int end_state = -1);
        ~CCDecoder();

        // Disable copy because of the raw pointers.
        CCDecoder(const CCDecoder &) = delete;
        CCDecoder &operator=(const CCDecoder &) = delete;

        void work(uint8_t *inbuffer, uint8_t *outbuffer);
        void work(uint8_t *inbuffer, uint8_t *outbuffer, int size);

        bool set_frame_size(unsigned int frame_size);
        double rate();
    };
}
