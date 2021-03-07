/* -*- c++ -*- */
/*
 * Copyright 2013-2014 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef INCLUDED_FEC_REINFLATE_BB_IMPL_H
#define INCLUDED_FEC_REINFLATE_BB_IMPL_H

#include <cstdint>
#include <vector>

namespace gr
{
    namespace fec
    {

        class depuncture_bb_impl
        {
        private:
            int d_puncsize;
            int d_delay;
            int d_puncholes;
            int d_puncpat;
            char d_sym;

        public:
            depuncture_bb_impl(int puncsize, int puncpat, int delay = 0, uint8_t symbol = 127);
            ~depuncture_bb_impl();

            int general_work(int ninput_items,
                             uint8_t *input_items,
                             uint8_t *output_items);
            int fixed_rate_ninput_to_noutput(int ninput);
            int fixed_rate_noutput_to_ninput(int noutput);
            void forecast(int noutput_items, int &ninput_items_required);

            std::vector<uint8_t> d_buffer;
        };

    } /* namespace fec */
} /* namespace gr */

#endif /* INCLUDED_FEC_DEPUNCTURE_BB_IMPL_H */
