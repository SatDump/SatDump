/**********************************************************************
 * This file is used for testing random stuff without running the 
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 * 
 * If you are an user, ignore this file which will not be built by 
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 * 
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include <iostream>

#include "common/codings/viterbi/cc_decoder_impl.h"

// Returns the asked bit!
template <typename T>
inline bool getBit(T &data, int &bit)
{
    return (data >> bit) & 1;
}

int main(int argc, char *argv[])
{
    uint8_t input_buffer[64 * 256];
    uint8_t output_buffer[32 * 256];

    uint64_t encoded_sync = /*0xfca2b63db11d9794; //*/ 0xfca2b63db00d9794;

    fec::code::cc_decoder_impl cc_decoder(32 * 256, 7, 2, {-79, -109}, 0, -1, CC_STREAMING, false);

    int t = 0;
    for (int y = 0; y < 256; y++)
    {
        for (int b = 63; b >= 0; b--)
            input_buffer[t++] = getBit<uint64_t>(encoded_sync, b) ? 200 : 0;
    }

    cc_decoder.generic_work(input_buffer, output_buffer);

    for (int y = 0; y < 256; y++)
    {
        uint32_t decoded = 0;

        for (int i = 0; i < 32; i++)
        {
            decoded = decoded << 1 | output_buffer[y * 32 + i];
        }

        std::cout << std::hex << decoded << std::endl;
    }
}