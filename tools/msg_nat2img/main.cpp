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

    if (argc < 3)
    {
        logger->error("Not enough arguments");
        return 1;
    }

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
    std::string mh_strs[48];
    mh_strs[0] = (char *)buf;
    mh_strs[1] = (char *)buf + 80;
    mh_strs[2] = (char *)buf + 160;
    mh_strs[3] = (char *)buf + 240;
    mh_strs[4] = (char *)buf + 320;
    mh_strs[5] = (char *)buf + 400;
    mh_strs[6] = (char *)buf + 480;
    mh_strs[6] += (char *)buf + 526;
    mh_strs[7] = (char *)buf + 542;
    mh_strs[7] += (char *)buf + 588;
    mh_strs[8] = (char *)buf + 604;
    mh_strs[8] += (char *)buf + 650;
    mh_strs[9] = (char *)buf + 666;
    mh_strs[9] += (char *)buf + 712;
    mh_strs[10] = (char *)buf + 728;
    mh_strs[10] += (char *)buf + 774;
    mh_strs[11] = (char *)buf + 2154;
    mh_strs[12] = (char *)buf + 2234;
    mh_strs[13] = (char *)buf + 2314;
    mh_strs[14] = (char *)buf + 2394;
    mh_strs[15] = (char *)buf + 2474;
    mh_strs[16] = (char *)buf + 2554;
    mh_strs[17] = (char *)buf + 2634;
    mh_strs[18] = (char *)buf + 2714;
    mh_strs[19] = (char *)buf + 2794;
    mh_strs[20] = (char *)buf + 2874;
    mh_strs[21] = (char *)buf + 2954;
    mh_strs[22] = (char *)buf + 3034;
    mh_strs[23] = (char *)buf + 3114;
    mh_strs[24] = (char *)buf + 3194;
    mh_strs[25] = (char *)buf + 3274;
    mh_strs[26] = (char *)buf + 3354;
    mh_strs[27] = (char *)buf + 3434;
    mh_strs[28] = (char *)buf + 3514;
    mh_strs[29] = (char *)buf + 3594;
    mh_strs[30] = (char *)buf + 3674;
    mh_strs[31] = (char *)buf + 3754;
    mh_strs[32] = (char *)buf + 3834;
    mh_strs[33] = (char *)buf + 3914;
    mh_strs[34] = (char *)buf + 3994;
    mh_strs[35] = (char *)buf + 4074;
    mh_strs[36] = (char *)buf + 4154;
    mh_strs[37] = (char *)buf + 4234;
    mh_strs[38] = (char *)buf + 4314;
    mh_strs[39] = (char *)buf + 4394;
    mh_strs[40] = (char *)buf + 4474;
    mh_strs[41] = (char *)buf + 4554;
    mh_strs[42] = (char *)buf + 4634;
    mh_strs[43] = (char *)buf + 4714;
    mh_strs[44] = (char *)buf + 4794;
    mh_strs[45] = (char *)buf + 4874;
    mh_strs[46] = (char *)buf + 4954;
    mh_strs[47] = (char *)buf + 5034;

    // for (int i = 0; i < 48; i++)
    //     logger->debug("L%d %s", i, mh_strs[i].substr(0, mh_strs[i].size() - 1));

    // IMG Size
    int vis_ir_x_size, vis_ir_y_size;
    int hrv_x_size, hrv_y_size;
    sscanf(mh_strs[44].c_str(), "NumberLinesVISIR            : %d", &vis_ir_y_size);
    sscanf(mh_strs[45].c_str(), "NumberColumnsVISIR          : %d", &vis_ir_x_size);
    sscanf(mh_strs[46].c_str(), "NumberLinesHRV              : %d", &hrv_y_size);
    sscanf(mh_strs[47].c_str(), "NumberColumnsHRV            : %d", &hrv_x_size);

    logger->warn("VIS/IR Size : %dx%d", vis_ir_x_size, vis_ir_y_size);
    logger->warn("HRV    Size : %dx%d", hrv_x_size, hrv_y_size);

    // Other data
    long int headerpos, datapos, trailerpos;
    sscanf(mh_strs[8].c_str(), "%*s : %*d %ld\n", &headerpos);
    sscanf(mh_strs[9].c_str(), "%*s : %*d %ld\n", &datapos);
    sscanf(mh_strs[10].c_str(), "%*s : %*d %ld\n", &trailerpos);

    char bandsel[16];
    sscanf(mh_strs[39].c_str(), "%*s : %s", bandsel);

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
                // uint16_t seq_cnt = (data_ptr + 16)[0] << 8 | (data_ptr + 16)[1];
                uint32_t pkt_len = (data_ptr + 18)[0] << 24 | (data_ptr + 18)[1] << 16 | (data_ptr + 18)[2] << 8 | (data_ptr + 18)[3];

                // logger->info("PKT %d CHANNEL %d SEQ %d LEN %d", line, channel, seq_cnt, pkt_len);

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

        std::string path = "SEVIRI_" + std::to_string(channel + 1);

        vis_ir_imgs[channel].mirror(true, true);

        logger->info("Saving " + path);
        vis_ir_imgs[channel].save_img(path);
    }

    std::string path = "SEVIRI_321";
    image::Image<uint16_t> compo_321(vis_ir_x_size, vis_ir_y_size, 3);
    compo_321.append_ext(&path);

    compo_321.draw_image(0, vis_ir_imgs[2]);
    compo_321.draw_image(1, vis_ir_imgs[1]);
    compo_321.draw_image(2, vis_ir_imgs[0]);

    logger->info("Saving " + path);
    compo_321.save_img(path);
}
