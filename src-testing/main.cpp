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

#include "logger.h"
#include <fstream>
#include "common/dsp/complex.h"
#include "volk/volk.h"
#include <volk/volk_alloc.hh>
#include "common/dsp/buffer.h"

int main(int /*argc*/, char *argv[])
{
    initLogger();

    std::ifstream data_in(argv[1], std::ios::binary);
    std::ofstream data_out(argv[2], std::ios::binary);

    complex_t *input_buffer = new complex_t[STREAM_BUFFER_SIZE];
    uint8_t *ziq_output_buffer = new uint8_t[STREAM_BUFFER_SIZE * 2];

    uint32_t pkt_size = 0;

    while (!data_in.eof())
    {
        data_in.read((char *)ziq_output_buffer, 12);

        pkt_size = *((uint32_t *)&ziq_output_buffer[4]);
        float scale = *((float *)&ziq_output_buffer[8]);

        data_in.read((char *)&ziq_output_buffer[12], pkt_size * 2);

        volk_8i_s32f_convert_32f((float *)input_buffer, (int8_t *)&ziq_output_buffer[12], scale, pkt_size * 2);

        data_out.write((char *)input_buffer, pkt_size * sizeof(complex_t));
    }
}