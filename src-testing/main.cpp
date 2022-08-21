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

int main(int argc, char *argv[])
{
    initLogger();
    std::ifstream input_frm(argv[1]);
    std::ofstream output_frm("test_mdl.frm");

    uint8_t cadu[4640];

    while (!input_frm.eof())
    {
        input_frm.read((char *)cadu, 464);

        int marker = (cadu[3] & 0b111) << 3 | (cadu[4] >> 5);

        logger->critical(marker);

        if (marker != 44)
            continue;

        // logger->info(cadu[marker * 30]);
        // if (cadu[marker * 30] == 168)
        output_frm.write((char *)cadu, 464);
    }
}
