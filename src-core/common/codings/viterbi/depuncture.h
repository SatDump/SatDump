/* -*- c++ -*- */
/*
 * Copyright 2013-2014 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#pragma once

#include <cstdint>
#include <volk/volk_alloc.hh>

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

        int general_work(int ninput_items, uint8_t *input_items, uint8_t *output_items);
        int fixed_rate_ninput_to_noutput(int ninput);
        int fixed_rate_noutput_to_ninput(int noutput);
        void forecast(int noutput_items, int &ninput_items_required);

        volk::vector<uint8_t> d_buffer;

        void clear()
        {
            d_buffer.clear();
        }
    };
} /* namespace fec */
