/* -*- c++ -*- */
/*
 * Copyright 2013-2014 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include "generic_decoder.h"

namespace fec
{

    generic_decoder::generic_decoder(std::string name)
    {
        d_name = name;
        my_id = base_unique_id++;
    }

    generic_decoder::~generic_decoder() {}

    int generic_decoder::get_history() { return 0; }

    float generic_decoder::get_shift() { return 0.0; }

    int generic_decoder::get_input_item_size() { return sizeof(float); }

    int generic_decoder::get_output_item_size() { return sizeof(char); }

    const char *generic_decoder::get_input_conversion() { return "none"; }

    const char *generic_decoder::get_output_conversion() { return "none"; }

    int generic_decoder::base_unique_id = 1;
    int generic_decoder::unique_id() { return my_id; }

    /*******************************************************
 * Static functions
 ******************************************************/
    int get_decoder_output_size(generic_decoder::sptr my_decoder)
    {
        return my_decoder->get_output_size();
    }

    int get_history(generic_decoder::sptr my_decoder) { return my_decoder->get_history(); }

    int get_decoder_input_size(generic_decoder::sptr my_decoder)
    {
        return my_decoder->get_input_size();
    }

    int get_decoder_output_item_size(generic_decoder::sptr my_decoder)
    {
        return my_decoder->get_output_item_size();
    }

    int get_decoder_input_item_size(generic_decoder::sptr my_decoder)
    {
        return my_decoder->get_input_item_size();
    }

    float get_shift(generic_decoder::sptr my_decoder) { return my_decoder->get_shift(); }

    const char *get_decoder_input_conversion(generic_decoder::sptr my_decoder)
    {
        return my_decoder->get_input_conversion();
    }

    const char *get_decoder_output_conversion(generic_decoder::sptr my_decoder)
    {
        return my_decoder->get_output_conversion();
    }

    int generic_decoder::continuous_work(uint8_t *in, int size, uint8_t *out)
    {
        d_buffer.insert(d_buffer.end(), &in[0], &in[size]);

        if (d_buffer.size() < get_input_size())
            return 0;

        int outsize = 0;
        uint8_t *input_ptr = d_buffer.data();
        uint8_t *output_ptr = out;

        while (d_buffer.size() >= get_input_size())
        {
            generic_work(input_ptr, output_ptr);

            input_ptr += get_input_size();
            output_ptr += get_output_size();
            outsize += get_output_size();

            d_buffer.erase(d_buffer.begin(), d_buffer.begin() + get_input_size());
        }

        return outsize;
    }

} /* namespace fec */