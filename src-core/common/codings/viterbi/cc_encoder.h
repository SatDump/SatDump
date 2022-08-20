#pragma once

#include "cc_common.h"
#include <map>
#include <string>

namespace viterbi
{
    class CCEncoder
    {
    private:
        unsigned char Partab[256];
        unsigned int d_frame_size;
        unsigned int d_max_frame_size;
        unsigned int d_rate;
        unsigned int d_k;
        std::vector<int> d_polys;
        unsigned int d_start_state;
        int d_output_size;

        int parity(int x);
        int parityb(unsigned char x);
        void partab_init(void);

    public:
        CCEncoder(int frame_size,
                        int k,
                        int rate,
                        std::vector<int> polys,
                        int start_state = 0);
        ~CCEncoder();

        bool set_frame_size(unsigned int frame_size);
        double rate();
        void work(uint8_t *inbuffer, uint8_t *outbuffer);
        void work(uint8_t *inbuffer, uint8_t *outbuffer, int size);
    };

}
