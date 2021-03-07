/* -*- c++ -*- */
/*
 * Copyright 2013-2014 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "depuncture_bb_impl.h"
//#include <gnuradio/io_signature.h>
//#include <pmt/pmt.h>
#include <volk/volk.h>
#include <cstdio>
#include <string>

namespace gr
{
    namespace fec
    {

        depuncture_bb_impl::depuncture_bb_impl(int puncsize,
                                               int puncpat,
                                               int delay,
                                               uint8_t symbol)
            : d_puncsize(puncsize),
              d_delay(delay),
              d_sym(symbol)
        {
            // Create a mask of all 1's of puncsize length
            int mask = 0;
            for (int i = 0; i < d_puncsize; i++)
                mask |= 1 << i;

            // Rotate the pattern for the delay value; then mask it if there
            // are any excess 1's in the pattern.
            for (int i = 0; i < d_delay; ++i)
            {
                puncpat = ((puncpat & 1) << (d_puncsize - 1)) + (puncpat >> 1);
            }
            d_puncpat = puncpat & mask;

            // Calculate the number of holes in the pattern. The mask is all
            // 1's given puncsize and puncpat is a pattern with >= puncsize
            // 0's (masked to ensure this). The difference between the
            // number of 1's in the mask and the puncpat is the number of
            // holes.
            uint32_t count_mask = 0, count_pat = 0;
            volk_32u_popcnt(&count_mask, static_cast<uint32_t>(mask));
            volk_32u_popcnt(&count_pat, static_cast<uint32_t>(d_puncpat));
            d_puncholes = count_mask - count_pat;

            // set_fixed_rate(true);
            // set_relative_rate((uint64_t)d_puncsize, (uint64_t)(d_puncsize - d_puncholes));
            // set_output_multiple(d_puncsize);
            // set_msg_handler(<portname>, [this](pmt::pmt_t msg) { this->catch_msg(msg); });
        }

        depuncture_bb_impl::~depuncture_bb_impl() {}

        int depuncture_bb_impl::fixed_rate_ninput_to_noutput(int ninput)
        {
            return std::lround((d_puncsize / (double)(d_puncsize - d_puncholes)) * ninput);
        }

        int depuncture_bb_impl::fixed_rate_noutput_to_ninput(int noutput)
        {
            return std::lround(((d_puncsize - d_puncholes) / (double)(d_puncsize)) * noutput);
        }

        void depuncture_bb_impl::forecast(int noutput_items, int &ninput_items_required)
        {
            ninput_items_required =
                std::lround(((d_puncsize - d_puncholes) / (double)(d_puncsize)) * noutput_items);
        }

        int depuncture_bb_impl::general_work(int ninput_items,
                                             uint8_t *input_items,
                                             uint8_t *output_items)
        {
            d_buffer.insert(d_buffer.end(), &input_items[0], &input_items[ninput_items]);

            int noutput_items = std::lround(d_buffer.size() / ((d_puncsize - d_puncholes) / (double)(d_puncsize)));

            int consumed = 0;
            for (int i = 0, k = 0; i < noutput_items / d_puncsize; ++i)
            {
                for (int j = 0; j < d_puncsize; ++j)
                {
                    output_items[i * d_puncsize + j] =
                        ((d_puncpat >> (d_puncsize - 1 - j)) & 1) ? d_buffer[k++] : d_sym;
                    if (((d_puncpat >> (d_puncsize - 1 - j)) & 1))
                        consumed++;
                }
            }

            d_buffer.erase(d_buffer.begin(), d_buffer.begin() + consumed);

            //   consume_each(std::lround((1.0 / relative_rate()) * noutput_items));
            return noutput_items;
        }

    } /* namespace fec */
} /* namespace gr */
