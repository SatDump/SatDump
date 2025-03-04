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

#include "common/ccsds/ccsds_tm/vcdu.h"
#include "common/ccsds/ccsds_tm/demuxer.h"
#include <fstream>
#include "common/simple_deframer.h"

#include "common/image/image.h"
#include "common/image/processing.h"
#include "common/image/io.h"

#include <cstring>

#include "common/repack.h"

#include "common/image/bayer/bayer.h"

#include "common/codings/reedsolomon/reedsolomon.h"

int main(int argc, char *argv[])
{
    initLogger();

    std::ifstream data_in(argv[1], std::ios::binary);

    uint8_t cadu[1279];

    ccsds::ccsds_tm::Demuxer demuxer_vcid1(1109, false, 0, 0);

    std::ofstream test_t("/tmp/test.bin");

    reedsolomon::ReedSolomon rs_check(reedsolomon::RS223);

    image::Image out;
    uint16_t data[2304* 1500] = {0};
    int counter = 0;

    while (!data_in.eof())
    {
        // Read buffer
        data_in.read((char *)&cadu, 1279);

        // Parse this transport frame
        ccsds::ccsds_tm::VCDU vcdu = ccsds::ccsds_tm::parseVCDU(cadu);

        //printf("VCID %d\n", vcdu.vcid);

        if (vcdu.vcid == 1)
        {
            auto pkts = demuxer_vcid1.work(cadu);

            for (auto pkt : pkts)
            {
                printf("APID %d \n", pkt.header.apid);
                //if (pkt.header.apid == 1031)
                {
                    // 1027 ?
                    // 1028 ?
                    // 1031 !!!!!
                    // 1032 !!!!!

                    //  printf("APID %d SIZE %d\n", pkt.header.apid, pkt.payload.size());

                    //pkt.payload.resize(1000);

                    test_t.write((char *)pkt.header.raw, 6);
                    test_t.write((char *)pkt.payload.data(), pkt.payload.size());
                    memcpy(data+counter*2304, (char *)pkt.payload.data()+15, 1536*2);
                    for (int i = 0; i < 3; i++){
                        for (int n = 0; n < 256; n++){
                            data[counter*2304+256*(i+6)+n] = data[counter*2304+256*(2*i)+n] + (int16_t)data[counter*2304+256*(2*i+1)+n];
                        }
                    }
                    counter++;
                }
            }
        }
    }
    out = image::Image(data, 16, 2304, 1500, 1);
    image::save_png(out, "/tmp/test.png"); 
}
