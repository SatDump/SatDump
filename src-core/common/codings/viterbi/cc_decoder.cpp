#include "cc_decoder.h"
#include <volk/volk.h>
#include <cmath>
#include "logger.h"
#include "volk_k7_r2_generic_fixed.h"
#include <cstring>

namespace viterbi
{
    CCDecoder::CCDecoder(int frame_size, int k, int rate, std::vector<int> polys, int start_state, int end_state)
        : d_k(k),
          d_rate(rate),
          d_polys(polys),
          d_start_state_chaining(start_state),
          d_start_state_nonchaining(start_state),
          d_end_state_nonchaining(end_state)
    {
        // Set max frame size here; all buffers and settings will be
        // based on this value.
        d_max_frame_size = frame_size;
        d_frame_size = frame_size;

        d_numstates = 1 << (d_k - 1);

        d_decision_t_size = d_numstates / 8; // packed bit array

        d_veclen = d_frame_size + d_k - 1;
        d_end_state = &d_end_state_chaining;

        d_vp.metrics.resize(2 * d_numstates);
        d_vp.metrics1.t = d_vp.metrics.data();
        d_vp.metrics2.t = d_vp.metrics.data() + d_numstates;

        d_vp.decisions.resize(d_veclen * d_decision_t_size);

        d_branchtab.resize(d_numstates / 2 * rate);

        create_viterbi();

        if (d_k - 1 < 8)
        {
            d_ADDSHIFT = (8 - (d_k - 1));
            d_SUBSHIFT = 0;
        }
        else if (d_k - 1 > 8)
        {
            d_ADDSHIFT = 0;
            d_SUBSHIFT = ((d_k - 1) - 8);
        }
        else
        {
            d_ADDSHIFT = 0;
            d_SUBSHIFT = 0;
        }

        conv_kernel k7_r2_kernel = volk_fixed::volk_8u_x4_conv_k7_r2_8u_generic; // Default to our fixed generic kernel

        // We need to get around Volk's broken generic and AVX kernel....
        {
            volk_func_desc k7_r2_desc = volk_8u_x4_conv_k7_r2_8u_get_func_desc(); // Check what kernels are available

            bool has_spiral = false, has_spiral_neon = false;

            for (int i = 0; i < (int)k7_r2_desc.n_impls; i++)
            {
                if (std::string(k7_r2_desc.impl_names[i]) == "spiral") // Try to find spiral
                {
                    has_spiral = true;
                    break;
                }
                if (std::string(k7_r2_desc.impl_names[i]) == "neonspiral") // Try to find neonspiral
                {
                    has_spiral_neon = true;
                    break;
                }
            }

            if (has_spiral)
            { // If spiral is available, use it
                logger->trace("Volk has the spiral kernel, using it!");
                k7_r2_kernel = volk_fixed::volk_8u_x4_conv_k7_r2_8u_spiral;
            }
            else if (has_spiral_neon)
            { // If spiral is available, use it
                logger->trace("Volk has the neonspiral kernel, using it!");
                k7_r2_kernel = volk_fixed::volk_8u_x4_conv_k7_r2_8u_neonspiral;
            }
            else
            { // Stick to our fixed kernel
                logger->trace("Volk does not have the spiral/neonspiral kernel, will default to bundled generic.");
            }
        }

        std::map<std::string, conv_kernel> yp_kernel = {{"k=7r=2", k7_r2_kernel}};

        std::string k_ = "k=";
        std::string r_ = "r=";

        std::ostringstream kerneltype;
        kerneltype << k_ << d_k << r_ << d_rate;

        d_kernel = yp_kernel[kerneltype.str()];
        if (d_kernel == NULL)
        {
            throw std::runtime_error("cc_decoder: parameters not supported");
        }
    }

    CCDecoder::~CCDecoder() {}

    void CCDecoder::create_viterbi()
    {
        int state;
        unsigned int i;
        partab_init();
        for (state = 0; state < d_numstates / 2; state++)
        {
            for (i = 0; i < d_rate; i++)
            {
                d_branchtab[i * d_numstates / 2 + state] =
                    (d_polys[i] < 0) ^ parity((2 * state) & abs(d_polys[i])) ? 255 : 0;
            }
        }

        d_start_state = &d_start_state_chaining;
        init_viterbi_unbiased(&d_vp);

        return;
    }

    int CCDecoder::parity(int x)
    {
        x ^= (x >> 16);
        x ^= (x >> 8);
        return parityb(x);
    }

    int CCDecoder::parityb(unsigned char x) { return Partab[x]; }

    void CCDecoder::partab_init(void)
    {
        int i, cnt, ti;

        /* Initialize parity lookup table */
        for (i = 0; i < 256; i++)
        {
            cnt = 0;
            ti = i;
            while (ti)
            {
                if (ti & 1)
                    cnt++;
                ti >>= 1;
            }
            Partab[i] = cnt & 1;
        }
    }

    int CCDecoder::init_viterbi(struct v *vp, int starting_state)
    {
        int i;

        if (vp == NULL)
            return -1;
        for (i = 0; i < d_numstates; i++)
        {
            vp->metrics1.t[i] = 63;
        }

        vp->old_metrics = vp->metrics1;
        vp->new_metrics = vp->metrics2;
        vp->old_metrics.t[starting_state & (d_numstates - 1)] =
            0; /* Bias known start state */
        return 0;
    }

    int CCDecoder::init_viterbi_unbiased(struct v *vp)
    {
        int i;

        if (vp == NULL)
            return -1;
        for (i = 0; i < d_numstates; i++)
            vp->metrics1.t[i] = 31;

        vp->old_metrics = vp->metrics1;
        vp->new_metrics = vp->metrics2;
        // no bias step
        return 0;
    }

    int CCDecoder::find_endstate()
    {
        unsigned char *met =
            ((d_k + d_veclen) % 2 == 0) ? d_vp.new_metrics.t : d_vp.old_metrics.t;

        unsigned char min = met[0];
        int state = 0;
        for (int i = 1; i < d_numstates; ++i)
        {
            if (met[i] < min)
            {
                min = met[i];
                state = i;
            }
        }
        // printf("min %d\n", state);
        return state;
    }

    int CCDecoder::update_viterbi_blk(unsigned char *syms, int nbits)
    {
        unsigned char *d = d_vp.decisions.data();

        memset(d, 0, d_decision_t_size * nbits);

        d_kernel(d_vp.new_metrics.t,
                 d_vp.old_metrics.t,
                 syms,
                 d,
                 nbits - (d_k - 1),
                 d_k - 1,
                 d_branchtab.data());

        return 0;
    }

    int CCDecoder::chainback_viterbi(unsigned char *data,
                                     unsigned int nbits,
                                     unsigned int endstate,
                                     unsigned int tailsize)
    {
        /* ADDSHIFT and SUBSHIFT make sure that the thing returned is a byte. */
        unsigned char *d = d_vp.decisions.data();
        /* Make room beyond the end of the encoder register so we can
         * accumulate a full byte of decoded data
         */

        endstate = (endstate % d_numstates) << d_ADDSHIFT;

        /* The store into data[] only needs to be done every 8 bits.
         * But this avoids a conditional branch, and the writes will
         * combine in the cache anyway
         */

        d += tailsize * d_decision_t_size; /* Look past tail */
        int retval = 0;
        int dif = tailsize - (d_k - 1);
        decision_t dec;
        while (nbits-- > d_frame_size - (d_k - 1))
        {
            int k;
            dec.t = &d[nbits * d_decision_t_size];
            k = (dec.w[(endstate >> d_ADDSHIFT) / 32] >> ((endstate >> d_ADDSHIFT) % 32)) & 1;

            endstate = (endstate >> 1) | (k << (d_k - 2 + d_ADDSHIFT));
            data[((nbits + dif) % d_frame_size)] = k;

            retval = endstate;
        }
        nbits += 1;

        while (nbits-- != 0)
        {
            int k;

            dec.t = &d[nbits * d_decision_t_size];

            k = (dec.w[(endstate >> d_ADDSHIFT) / 32] >> ((endstate >> d_ADDSHIFT) % 32)) & 1;

            endstate = (endstate >> 1) | (k << (d_k - 2 + d_ADDSHIFT));
            data[((nbits + dif) % d_frame_size)] = k;
        }

        return retval >> d_ADDSHIFT;
    }

    bool CCDecoder::set_frame_size(unsigned int frame_size)
    {
        bool ret = true;
        if (frame_size > d_max_frame_size)
        {
            frame_size = d_max_frame_size;
            ret = false;
        }

        d_frame_size = frame_size;
        d_veclen = d_frame_size + d_k - 1;

        return ret;
    }

    double CCDecoder::rate() { return 1.0 / static_cast<double>(d_rate); }

    void CCDecoder::work(uint8_t *in, uint8_t *out)
    {
        update_viterbi_blk((unsigned char *)(&in[0]), d_veclen);
        d_end_state_chaining = find_endstate();
        d_start_state_chaining = chainback_viterbi(&out[0], d_frame_size, *d_end_state, d_veclen - d_frame_size);

        init_viterbi(&d_vp, *d_start_state);
    }

    void CCDecoder::work(uint8_t *in, uint8_t *out, int size)
    {
        int frm_size = size / d_k;
        int veclen = frm_size + d_k - 1;

        update_viterbi_blk((unsigned char *)(&in[0]), d_veclen);
        d_end_state_chaining = find_endstate();
        d_start_state_chaining = chainback_viterbi(&out[0], d_frame_size, *d_end_state, veclen - frm_size);

        init_viterbi(&d_vp, *d_start_state);
    }
}
