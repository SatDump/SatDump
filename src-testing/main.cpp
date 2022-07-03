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
#include "common/image/image.h"
#include "common/repack.h"

int main(int argc, char *argv[])
{
    initLogger();

    std::string msg_native_file = argv[1]; //"/home/alan/Downloads/MSG4-SEVI-MSG15-0100-NA-20220701121243.901000000Z-NA/MSG4-SEVI-MSG15-0100-NA-20220701121243.901000000Z-NA.nat";

    // We will in the future want to decode from memory, so load it all up in RAM
    std::vector<uint8_t> msg_file;
    {
        std::ifstream input_file(msg_native_file);
        uint8_t byte;
        while (!input_file.eof())
        {
            input_file.read((char *)&byte, 1);
            msg_file.push_back(byte);
        }
    }

    uint8_t *buf = msg_file.data();

    // Parse header
    std::string mphinfo[48];
    mphinfo[0] = (char *)buf;
    mphinfo[1] = (char *)buf + 80;
    mphinfo[2] = (char *)buf + 160;
    mphinfo[3] = (char *)buf + 240;
    mphinfo[4] = (char *)buf + 320;
    mphinfo[5] = (char *)buf + 400;
    mphinfo[6] = (char *)buf + 480;
    mphinfo[6] += (char *)buf + 526;
    mphinfo[7] = (char *)buf + 542;
    mphinfo[7] += (char *)buf + 588;
    mphinfo[8] = (char *)buf + 604;
    mphinfo[8] += (char *)buf + 650;
    mphinfo[9] = (char *)buf + 666;
    mphinfo[9] += (char *)buf + 712;
    mphinfo[10] = (char *)buf + 728;
    mphinfo[10] += (char *)buf + 774;
    mphinfo[11] = (char *)buf + 2154;
    mphinfo[12] = (char *)buf + 2234;
    mphinfo[13] = (char *)buf + 2314;
    mphinfo[14] = (char *)buf + 2394;
    mphinfo[15] = (char *)buf + 2474;
    mphinfo[16] = (char *)buf + 2554;
    mphinfo[17] = (char *)buf + 2634;
    mphinfo[18] = (char *)buf + 2714;
    mphinfo[19] = (char *)buf + 2794;
    mphinfo[20] = (char *)buf + 2874;
    mphinfo[21] = (char *)buf + 2954;
    mphinfo[22] = (char *)buf + 3034;
    mphinfo[23] = (char *)buf + 3114;
    mphinfo[24] = (char *)buf + 3194;
    mphinfo[25] = (char *)buf + 3274;
    mphinfo[26] = (char *)buf + 3354;
    mphinfo[27] = (char *)buf + 3434;
    mphinfo[28] = (char *)buf + 3514;
    mphinfo[29] = (char *)buf + 3594;
    mphinfo[30] = (char *)buf + 3674;
    mphinfo[31] = (char *)buf + 3754;
    mphinfo[32] = (char *)buf + 3834;
    mphinfo[33] = (char *)buf + 3914;
    mphinfo[34] = (char *)buf + 3994;
    mphinfo[35] = (char *)buf + 4074;
    mphinfo[36] = (char *)buf + 4154;
    mphinfo[37] = (char *)buf + 4234;
    mphinfo[38] = (char *)buf + 4314;
    mphinfo[39] = (char *)buf + 4394;
    mphinfo[40] = (char *)buf + 4474;
    mphinfo[41] = (char *)buf + 4554;
    mphinfo[42] = (char *)buf + 4634;
    mphinfo[43] = (char *)buf + 4714;
    mphinfo[44] = (char *)buf + 4794;
    mphinfo[45] = (char *)buf + 4874;
    mphinfo[46] = (char *)buf + 4954;
    mphinfo[47] = (char *)buf + 5034;

    for (int i = 0; i < 48; i++)
        logger->debug("L{:d} {:s}", i, mphinfo[i].substr(0, mphinfo[i].size() - 1));

    // IMG Size
    int vis_ir_x_size, vis_ir_y_size;
    int hrv_x_size, hrv_y_size;
    sscanf(mphinfo[44].c_str(), "NumberLinesVISIR            : %d", &vis_ir_y_size);
    sscanf(mphinfo[45].c_str(), "NumberColumnsVISIR          : %d", &vis_ir_x_size);
    sscanf(mphinfo[46].c_str(), "NumberLinesHRV              : %d", &hrv_y_size);
    sscanf(mphinfo[47].c_str(), "NumberColumnsHRV            : %d", &hrv_x_size);

    logger->critical("VIS/IR Size : {:d}x{:d}", vis_ir_x_size, vis_ir_y_size);
    logger->critical("HRV    Size : {:d}x{:d}", hrv_x_size, hrv_y_size);

    // Other data
    long int headerpos, datapos, trailerpos;
    sscanf(mphinfo[8].c_str(), "%*s : %*d %ld\n", &headerpos);
    sscanf(mphinfo[9].c_str(), "%*s : %*d %ld\n", &datapos);
    sscanf(mphinfo[10].c_str(), "%*s : %*d %ld\n", &trailerpos);

    char bandsel[16];
    sscanf(mphinfo[39].c_str(), "%*s : %s", bandsel);

    image::Image<uint16_t> vis_ir_imgs[12];
    for (int channel = 0; channel < 12; channel++)
    {
        if (bandsel[channel] != 'X')
            continue;

        vis_ir_imgs[channel].init((channel == 11 ? hrv_x_size : vis_ir_x_size), (channel == 11 ? hrv_y_size : vis_ir_y_size), 1);
    }

    uint16_t repacked_line[81920];

    // Read line data
    // uint8_t pkt_hdr[48];
    uint8_t *data_ptr = buf + datapos;
    for (int line = 0; line < vis_ir_y_size; line++)
    {
        for (int channel = 0; channel < 12; channel++)
        {
            if (bandsel[channel] != 'X')
                continue;

            for (int i = 0; i < (channel == 11 ? 3 : 1); i++)
            {
                uint16_t seq_cnt = (data_ptr + 16)[0] << 8 | (data_ptr + 16)[1];
                uint32_t pkt_len = (data_ptr + 18)[0] << 24 | (data_ptr + 18)[1] << 16 | (data_ptr + 18)[2] << 8 | (data_ptr + 18)[3];

                // logger->info("PKT {:d} CHANNEL {:d} SEQ {:d} LEN {:d}", line, channel, seq_cnt, pkt_len);

                int datasize = pkt_len - 15 - 27;

                repackBytesTo10bits(data_ptr + 38 + 27, datasize, repacked_line);

                for (int x = 0; x < (channel == 11 ? hrv_x_size : vis_ir_x_size); x++)
                    vis_ir_imgs[channel][(channel == 11 ? (line * 3 + i) : line) * (channel == 11 ? hrv_x_size : vis_ir_x_size) + x] = repacked_line[x] << 6;

                data_ptr += 38 + 27 + datasize;
            }
        }
    }

    // Saving
    for (int channel = 0; channel < 12; channel++)
    {
        if (bandsel[channel] != 'X')
            continue;

        std::string path = "SEVIRI_" + std::to_string(channel + 1) + ".png";

        vis_ir_imgs[channel].mirror(true, true);

        logger->info("Saving " + path);
        vis_ir_imgs[channel].save_png(path);
    }

    std::string path = "SEVIRI_321.png";

    image::Image<uint16_t> compo_321(vis_ir_x_size, vis_ir_y_size, 3);

    compo_321.draw_image(0, vis_ir_imgs[2]);
    compo_321.draw_image(1, vis_ir_imgs[1]);
    compo_321.draw_image(2, vis_ir_imgs[0]);

    logger->info("Saving " + path);
    compo_321.save_png(path);
}
