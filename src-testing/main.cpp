/**********************************************************************
 * This file is used for testing random stuff without running the 
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 * 
 * If you are an user, ignore this file which will not be built by 
 * default, and you're a developper in need of doing stuff here...
 * Go ahead!
 * 
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include <iostream>
#include <fstream>
#include "modules/common/sathelper/packetfixer.h"
#include "modules/viterbi/cc_decoder_impl.h"
#include "modules/viterbi/cc_encoder_impl.h"
#include "modules/viterbi/depuncture_bb_impl.h"
#include "modules/common/repack_bits_byte.h"

void char_array_to_uchar(const char *in, unsigned char *out, int nsamples)
{
    for (int i = 0; i < nsamples; i++)
    {
        long int r = (long int)rint((float)in[i] * 127.0);
        if (r < 0)
            r = 0;
        else if (r > 255)
            r = 255;
        out[i] = r;
    }
}

#define BUFFER_SIZE 8192
#define TEST_SIZE 1024

int main(int argc, char *argv[])
{
    uint8_t input_buffer[BUFFER_SIZE];
    uint8_t input_buffer_converted[BUFFER_SIZE];
    uint8_t reorg_buffer[BUFFER_SIZE];
    uint8_t depunc_buffer[BUFFER_SIZE * 2];
    uint8_t encode_buffer[BUFFER_SIZE * 2];
    uint8_t output_buffer[BUFFER_SIZE * 2];
    uint8_t byte_buffer[BUFFER_SIZE * 2];

    gr::fec::depuncture_bb_impl depunc(3, 110);
    gr::fec::code::cc_decoder_impl cc_decoder(8192, 7, 2, {79, 109}, 0, -1, CC_STREAMING, false);
    RepackBitsByte repacker;

    /*gr::fec::depuncture_bb_impl depunc_ber(3, 110);
    gr::fec::code::cc_decoder_impl cc_decoder_ber((TEST_SIZE * 1.5f) / 2.0f, 7, 2, {79, 109}, 0, -1, CC_STREAMING, false);
    gr::fec::code::cc_encoder_impl cc_encoder_ber((TEST_SIZE * 1.5f) / 2.0f, 7, 2, {79, 109}, 0, CC_STREAMING, false);
    uint8_t ber_in_buffer[TEST_SIZE];
    uint8_t ber_converted_buffer[TEST_SIZE];
    uint8_t ber_reorg_buffer[TEST_SIZE];
    uint8_t ber_depunc_buffer[TEST_SIZE * 2];
    uint8_t ber_decode_buffer[TEST_SIZE];
    uint8_t ber_encode_buffer[TEST_SIZE * 2];*/

    std::ifstream data_in("test.soft", std::ios::binary);
    std::ofstream data_out("test.bin", std::ios::binary);

    while (!data_in.eof())
    {
        data_in.read((char *)input_buffer, BUFFER_SIZE);

        /*{
            std::memcpy(ber_in_buffer, input_buffer, TEST_SIZE);
            char_array_to_uchar((char *)ber_in_buffer, ber_converted_buffer, TEST_SIZE);

            for (int i = 0; i < TEST_SIZE / 4; i++)
            {
                ber_reorg_buffer[i * 4 + 0] = ber_converted_buffer[i * 4 + 0];
                ber_reorg_buffer[i * 4 + 1] = ber_converted_buffer[i * 4 + 1];
                ber_reorg_buffer[i * 4 + 2] = ber_converted_buffer[i * 4 + 3];
                ber_reorg_buffer[i * 4 + 3] = ber_converted_buffer[i * 4 + 2];
            }

            depunc.general_work(TEST_SIZE, ber_reorg_buffer, ber_depunc_buffer);

            cc_decoder_ber.generic_work(ber_depunc_buffer, ber_decode_buffer);
            cc_encoder_ber.generic_work(ber_decode_buffer, ber_encode_buffer);

            float errors = 0;
            for (int i = 0; i < TEST_SIZE * 1.5f; i++)
                errors += (ber_depunc_buffer[i] > 0) != (ber_encode_buffer[i] > 0);

            std::cout << (errors / ((float)TEST_SIZE * 1.5f * 2.0f)) * 2.0f << '\n';
        }*/

        char_array_to_uchar((char *)input_buffer, input_buffer_converted, BUFFER_SIZE);

        for (int i = 0; i < BUFFER_SIZE / 4; i++)
        {
            reorg_buffer[i * 4 + 0] = input_buffer_converted[i * 4 + 0];
            reorg_buffer[i * 4 + 1] = input_buffer_converted[i * 4 + 1];
            reorg_buffer[i * 4 + 2] = input_buffer_converted[i * 4 + 3];
            reorg_buffer[i * 4 + 3] = input_buffer_converted[i * 4 + 2];
        }

        depunc.general_work(BUFFER_SIZE, reorg_buffer, depunc_buffer);

        int out = cc_decoder.continuous_work(depunc_buffer, BUFFER_SIZE * 1.5, output_buffer);

        // std::cout << decoded << std::endl;

        int bytes_out = repacker.work(output_buffer, out, byte_buffer);

        data_out.write((char *)byte_buffer, bytes_out);
    }
}