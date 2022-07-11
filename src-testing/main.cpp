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
#include "common/mpeg_ts/ts_header.h"
#include "common/mpeg_ts/ts_demux.h"
#include <map>
#include "libs/bzlib/bzlib.h"
#include "common/mpeg_ts/fazzt_processor.h"

#include "common/image/image.h"
#include <filesystem>

int main(int argc, char *argv[])
{
    initLogger();

    std::ifstream tmp_file_decomp(argv[1]);

    std::vector<uint8_t> data;
    while (!tmp_file_decomp.eof())
    {
        uint8_t b;
        tmp_file_decomp.read((char *)&b, 1);
        data.push_back(b);
    }

    unsigned int outsize = 150 * 1e6; // 200MB Buffer
    uint8_t *out_buffer = new uint8_t[outsize];

    int ret = BZ2_bzBuffToBuffDecompress((char *)out_buffer, &outsize, (char *)&data[0], data.size(), false, 1);
    if (ret != BZ_OK && ret != BZ_STREAM_END)
    {
        logger->error("Failed decomressing Bzip2 data! Error : " + std::to_string(ret));
        return 1;
    }

    std::ofstream tmp_file_comp(argv[2]);
    tmp_file_comp.write((char *)out_buffer, 550 * 1e6);
    tmp_file_comp.close();
}