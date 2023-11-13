#include "logger.h"

#include <fstream>

#include "common/codings/randomization.h"
#include "common/codings/reedsolomon/reedsolomon.h"

int main(int argc, char *argv[])
{
    initLogger();

    if (argc < 3)
        return 1;

    std::ifstream data_in(argv[1], std::ios::binary);
    std::ofstream data_out(argv[2], std::ios::binary);

    int frame_size = std::stoi(argv[3]);
    int rs_size = std::stoi(argv[4]);

    uint8_t *cadu = new uint8_t[frame_size];

    reedsolomon::ReedSolomon rsdec(reedsolomon::RS223);

    while (!data_in.eof())
    {
        data_in.read((char *)cadu, frame_size);

        derand_ccsds(&cadu[4], frame_size - 4);

        int errors[4];
        rsdec.decode_interlaved(&cadu[4], true, rs_size, errors);

        data_out.write((char *)cadu, frame_size);
    }

    delete[] cadu;
}
