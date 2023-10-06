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
#include <vector>
#include "main.h"

int main(int argc, char *argv[])
{
    initLogger();

    if (argc < 3)
    {
        logger->error("Not enough arguments");
        return 1;
    }

    std::string native_file = argv[1];
    std::string pro_output_file = argv[2];

    // We will in the future want to decode from memory, so load it all up in RAM
    std::vector<uint8_t> nat_file;
    {
        std::ifstream input_file(native_file);
        uint8_t byte;
        while (!input_file.eof())
        {
            input_file.read((char *)&byte, 1);
            nat_file.push_back(byte);
        }
    }

    char *identifier = (char *)&nat_file[0];
    if (identifier[0] == 'F' &&
        identifier[1] == 'o' &&
        identifier[2] == 'r' &&
        identifier[3] == 'm' &&
        identifier[4] == 'a' &&
        identifier[5] == 't' &&
        identifier[6] == 'N' &&
        identifier[7] == 'a' &&
        identifier[8] == 'm' &&
        identifier[9] == 'e')
        decodeMSGNat(nat_file, pro_output_file);
    else if (identifier[552] == 'A' &&
             identifier[553] == 'V' &&
             identifier[554] == 'H' &&
             identifier[555] == 'R')
        decodeAVHRRNat(nat_file, pro_output_file);
    else
        logger->error("Uknown File Type!");
}
