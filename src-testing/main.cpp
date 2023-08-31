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
#include <cstring>
#include "common/codings/differential/nrzm.h"

#include "common/simple_deframer.h"

#include "common/image/image.h"

uint8_t reverseBits(uint8_t byte)
{
    byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
    byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
    byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
    return byte;
}

int main(int argc, char *argv[])
{
    initLogger();

    uint8_t *softs_buffer = new uint8_t[3072];
    uint8_t *frame_buffer = new uint8_t[3072];

    std::ifstream data_in(argv[1], std::ios::binary);
    std::ofstream data_out(argv[2], std::ios::binary);

    diff::NRZMDiff diff1, diff2;

    bool synced = 0;
    int runs_since_sync = 0;

    def::SimpleDeframer deframer(0x38fb456a, 32, 3040, 0, false); // 0xb6f23041fc

    image::Image<uint16_t> image_idk[92];

    for (int v = 0; v < 92; v++)
        image_idk[v].init(300, 8000, 1);

    int lines_img = 0;

    int last_vt = 0;

    while (!data_in.eof()) //&& lines_img < 500)
    {
        memmove(&softs_buffer[0], &softs_buffer[1], 3072 - 1);
        data_in.read((char *)&softs_buffer[3072 - 1], 1);

        for (int i = 0; i < 3072; i++)
            frame_buffer[i] = softs_buffer[i]; // >= 0;

        bool sync1 = frame_buffer[0] == 1 &&
                     frame_buffer[1] == 0 &&
                     frame_buffer[2] == 1 &&
                     frame_buffer[3] == 1 &&
                     frame_buffer[4] == 0 &&
                     frame_buffer[5] == 0 &&
                     frame_buffer[6] == 1 &&
                     frame_buffer[7] == 1;

        bool sync2 = frame_buffer[384 + 0] == 1 &&
                     frame_buffer[384 + 1] == 1 &&
                     frame_buffer[384 + 2] == 1 &&
                     frame_buffer[384 + 3] == 0 &&
                     frame_buffer[384 + 4] == 0 &&
                     frame_buffer[384 + 5] == 0 &&
                     frame_buffer[384 + 6] == 1 &&
                     frame_buffer[384 + 7] == 1;

        bool sync3 = frame_buffer[768 + 0] == 0 &&
                     frame_buffer[768 + 1] == 1 &&
                     frame_buffer[768 + 2] == 1 &&
                     frame_buffer[768 + 3] == 1 &&
                     frame_buffer[768 + 4] == 0 &&
                     frame_buffer[768 + 5] == 1 &&
                     frame_buffer[768 + 6] == 0 &&
                     frame_buffer[768 + 7] == 1;

        bool sync4 = frame_buffer[1920 + 0] == 0 &&
                     frame_buffer[1920 + 1] == 0 &&
                     frame_buffer[1920 + 2] == 0 &&
                     frame_buffer[1920 + 3] == 0 &&
                     frame_buffer[1920 + 4] == 0 &&
                     frame_buffer[1920 + 5] == 0 &&
                     frame_buffer[1920 + 6] == 0 &&
                     frame_buffer[1920 + 8] == 0;

        runs_since_sync++;

        if (synced ? (sync1 && sync2 && sync3 && sync4) : ((int)sync1 + (int)sync2 + (int)sync3 + (int)sync4) > 2)
        {
            uint8_t bytes[384];
            for (int i = 0; i < 3072; i++)
                bytes[i / 8] = bytes[i / 8] << 1 | softs_buffer[i];

                // diff1.decode(bytes, 384);

#if 0
            data_out.write((char *)bytes, 3072 / 8);
#else
            /*
            data_out.write((char *)bytes + 2, 46);
            data_out.write((char *)bytes + 50, 46);
            data_out.write((char *)bytes + 98, 46);
            data_out.write((char *)bytes + 146, 46);
            data_out.write((char *)bytes + 194, 46);
            data_out.write((char *)bytes + 242, 46);
            data_out.write((char *)bytes + 290, 46);
            data_out.write((char *)bytes + 338, 46);
            */

            std::vector<uint8_t> deblocked_data;

            deblocked_data.insert(deblocked_data.end(), bytes + 2, bytes + 2 + 46);
            deblocked_data.insert(deblocked_data.end(), bytes + 50, bytes + 50 + 46);
            deblocked_data.insert(deblocked_data.end(), bytes + 98, bytes + 98 + 46);
            deblocked_data.insert(deblocked_data.end(), bytes + 146, bytes + 146 + 46);
            deblocked_data.insert(deblocked_data.end(), bytes + 194, bytes + 194 + 46);
            deblocked_data.insert(deblocked_data.end(), bytes + 242, bytes + 242 + 46);
            deblocked_data.insert(deblocked_data.end(), bytes + 290, bytes + 290 + 46);
            deblocked_data.insert(deblocked_data.end(), bytes + 338, bytes + 338 + 46);
#endif

            for (int v = 0; v < deblocked_data.size(); v++)
                deblocked_data[v] ^= 0xAA; // 0x55; // reverseBits(frm[v]) ^ 0xFF; //   frm[v] ^= 0xFF;

            // 158080 0x9251efc0ab

            //  data_in.read((char *)&softs_buffer[1], 3072 - 1);

            // data_out.write((char *)deblocked_data.data(), deblocked_data.size());

            auto deframed_dat = deframer.work(deblocked_data.data(), deblocked_data.size());

            for (auto &frm : deframed_dat)
            {
                // diff::nrzm_decode(frm.data(), 158080 / 8);
                //  for (int i = 0; i < 200; i++)
                //      data_out.write((char *)frm.data() + 10 + i * 92, 2);

                int counter = frm[4];

                if (counter == 0)
                {
                    data_out.write((char *)frm.data(), 3040 / 8);
                    uint64_t vt = // frm[20] << 40 |
                                  // frm[21] << 32 |
                        // frm[24] << 24 |
                        // frm[25] << 16 |
                        frm[6] << 8 |
                        frm[7] << 0;
                    // vt /= 1e4;
                    double v1 = last_vt;
                    double v2 = vt;
                    int h = vt / 3600;
                    int m = (vt % 3600) / 60;
                    logger->trace("%f ---- %f           %dh %dmin", v2, v2 - v1, h, m);
                    last_vt = vt;
                }

                // data_out.write((char *)line, 300 * 2);

                int pos = 10;
                for (int c = 0; c < 46; c++)
                {
                    for (int p = 0; p < 4; p++)
                    {
                        uint16_t val =
                            (frm[pos + p * 92 + c * 2 + 0] << 8 | frm[pos + p * 92 + c * 2 + 1]); //+ 32768;

                        // int16_t val2 = *((int16_t *)&val);

                        image_idk[c][lines_img * image_idk->width() + counter * 4 + p] = val; // (int)val2; // + 32768;
                    }
                }

                if (counter == 51)
                {
                    lines_img++;

                    logger->info("Lines %d CNT %d", lines_img, counter);
                }
            }

            if (runs_since_sync > 2 && !synced)
            {
                synced = true;
                runs_since_sync = 0;
                // logger->critical("SYNC!");
            }
        }
        else
        {
            runs_since_sync++;
            synced = false;
        }
    }

    for (int v = 0; v < 46; v++)
    {
        image_idk[v].crop(0, 0, image_idk[v].width(), lines_img);
        image_idk[v].crop(4, 4 + 200);
        image_idk[v].mirror(true, false);
        // image_idk[v].linear_invert();
        image_idk[v].equalize();
        image_idk[v].save_png("meteor_dump/image_idk_" + std::to_string(v) + ".png");
    }
}