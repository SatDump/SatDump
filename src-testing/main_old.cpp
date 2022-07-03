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

#if 0
#include "logger.h"
#include "products/image_products.h"

#include "common/projection/sat_proj/sat_proj.h"

int main(int argc, char *argv[])
{
    initLogger();

    std::string user_path = std::string(getenv("HOME")) + "/.config/satdump";
    //satdump::config::loadConfig("satdump_cfg.json", user_path);

    satdump::ImageProducts img_pro;
    img_pro.load(argv[1]);

    std::shared_ptr<satdump::SatelliteProjection> sat_projection = get_sat_proj(img_pro.get_proj_cfg(), img_pro.get_tle(), img_pro.get_timestamps());
}
#else
#include "logger.h"
#include <fstream>
#include "common/codings/dvb-s2/bbframe_bch.h"
#include "common/codings/dvb-s2/bbframe_ldpc.h"

int main(int argc, char *argv[])
{
    initLogger();

    dvbs2::BBFrameBCH bch_handler;
    dvbs2::dvbs2_framesize_t s2_framesize = dvbs2::FECFRAME_NORMAL;
    dvbs2::dvbs2_code_rate_t s2_code_rate = dvbs2::C9_10;

    int frame_size = bch_handler.frame_params(s2_framesize, s2_code_rate).second / 8;
    uint8_t frame_data[frame_size];
    size_t total_filesize_bytes = 0;

    std::vector<uint8_t> test_data;

    // Generate test data
    logger->info("Generating test data...");
    {
        for (int i = 0; i < 1e4; i++)
        {
            for (int y = 0; y < bch_handler.frame_params(s2_framesize, s2_code_rate).first / 8; y++)
                frame_data[y] = rand() % 256;
            bch_handler.encode(frame_data, s2_framesize, s2_code_rate);
            // output_file.write((char *)frame_data, frame_size);
            test_data.insert(test_data.end(), &frame_data[0], &frame_data[frame_size]);
            total_filesize_bytes += frame_size;
        }
    }

    logger->info("Decoding... (Test Size {:.1f} MB)", total_filesize_bytes / 1e6);

    {
        int ff = 0;

        auto bench_start = std::chrono::system_clock::now();
        while (ff < test_data.size() / frame_size)
        {
            // input_file.read((char *)frame_data, frame_size);
            uint8_t *frame_ptr = &test_data[(ff++) * frame_size];
            bch_handler.decode(frame_ptr, s2_framesize, s2_code_rate);
        }

        auto bench_time = (std::chrono::system_clock::now() - bench_start);
        double time = bench_time.count() / 1e9;

        logger->debug("Processing Time {:f}, throughput {:f} MB/s", time, double(total_filesize_bytes / 1e6) / time);
    }
}

#endif