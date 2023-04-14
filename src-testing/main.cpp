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

#include "common/image/image.h"
#include "resources.h"
#include <vector>

#include "common/codings/reedsolomon/reedsolomon.h"
#include <fstream>

int main(int argc, char *argv[])
{
    initLogger();

    std::ifstream data_in(argv[1], std::ios::binary);
    std::ofstream data_out(argv[2], std::ios::binary);

    reedsolomon::ReedSolomon reed_solomon(reedsolomon::RS223);

    int errors[5];

    uint8_t cadu[1279];
    while (!data_in.eof())
    {
        data_in.read((char *)cadu, 1279);

        reed_solomon.decode_interlaved(&cadu[4], true, 5, errors);

        for (int i = 0; i < 5; i++)
            if (errors[i] != 0)
                printf("ERROR\n");

        data_out.write((char *)cadu, 1279);
    }
}
