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

#include "common/ccsds/ccsds_tm/demuxer.h"
#include "common/ccsds/ccsds_tm/vcdu.h"
#include "common/simple_deframer.h"
#include <fstream>

#include "common/image/image.h"
#include "common/image/io.h"
#include "common/image/processing.h"

#include <cstring>

#include "common/repack.h"

#include "common/image/bayer/bayer.h"

#include "common/codings/reedsolomon/reedsolomon.h"

int main(int argc, char *argv[])
{
    initLogger();

    std::ifstream data_in(argv[1], std::ios::binary);

    std::ofstream test_t(argv[2]);

    uint8_t frm[528];

    const uint8_t pm_sequence[] = {0xad, 0x43, 0xc4, 0x7e, 0x31, 0x6c, 0x28, 0xae, //
                                   0xde, 0x63, 0xd0, 0x93, 0x2f, 0x10, 0xf0, 0x07, //
                                   0xc2, 0x0e, 0x8c, 0xdf, 0x6b, 0x12, 0xe1, 0x83, //
                                   0x27, 0x56, 0xe3, 0x92, 0xa3, 0xb3, 0xbb, 0xfd, //
                                   0x6e, 0x7b, 0x1a, 0xa7, 0x90, 0xb2, 0x37, 0x5e, //
                                   0xa5, 0x81, 0x36, 0xd2, 0x06, 0xca, 0xcc, 0x7e, //
                                   0x73, 0x5c, 0xb4, 0x05, 0xd3, 0x8a, 0x69, 0x87, //
                                   0x04, 0x5f, 0x29, 0x22};

    while (!data_in.eof())
    {
        // Read buffer
        data_in.read((char *)frm, 528);

        memset(frm, 0, 528);
        memcpy(frm, pm_sequence, 60);

        test_t.write((char *)frm, 528);
    }
}
