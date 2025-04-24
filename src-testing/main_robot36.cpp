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

#include "common/image/image.h"
#include "common/image/io.h"
#include "common/utils.h"
#include "logger.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>

std::vector<uint8_t> genSeq()
{
    std::vector<uint8_t> t;
    for (int i = 0; i < 504; i++)
        t.push_back(0);
    return t;
}

void GetColorFromYCbCr(int y, int cb, int cr, int &_r, int &_g, int &_b)
{
    double Y = (double)y;
    double Cb = (double)cb;
    double Cr = (double)cr;

    int r = (int)(Y + 1.40200 * (Cr - 0x80));
    int g = (int)(Y - 0.34414 * (Cb - 0x80) - 0.71414 * (Cr - 0x80));
    int b = (int)(Y + 1.77200 * (Cb - 0x80));

    _r = std::max(0, std::min(255, r));
    _g = std::max(0, std::min(255, g));
    _b = std::max(0, std::min(255, b));
}

int clamp(int v)
{
    if (v > 255)
        v = 255;
    if (v < 0)
        v = 0;
    return v;
}

void YUV2RGB(int Y, int U, int V, int &R, int &G, int &B)
{
    Y -= 16;
    U -= 128;
    V -= 128;
    R = clamp((298 * Y + 409 * V + 128) >> 8);
    G = clamp((298 * Y - 100 * U - 208 * V + 128) >> 8);
    B = clamp((298 * Y + 516 * U + 128) >> 8);
}

float calcValFromFreq(float freq) { return (2. * M_PI * freq) / 48000.0; }

int main(int argc, char *argv[])
{
    initLogger();

    std::ifstream data_in("/home/alan/Downloads/SSTV/sstv_out_cdemod.f32");

    float minval = calcValFromFreq(1200);
    float minvalimg = calcValFromFreq(1488); // TODO should be 1500?
    float maxval = calcValFromFreq(2300);

    std::vector<uint8_t> wip_img;
    while (!data_in.eof())
    {
        float v;
        data_in.read((char *)&v, sizeof(float));

        v = (v - minval) / (maxval - minval);

        if (v < 0)
            v = 0;
        if (v > 1)
            v = 1;
        wip_img.push_back(v * 255);
    }

    int line_length = 0.150 * 48000;
    int line_cnt = wip_img.size() / line_length;

    image::Image newimg(wip_img.data(), 8, line_length, line_cnt, 1);
    image::Image newimg_vs = newimg;
    newimg_vs.to_rgb();

    auto sync = genSeq();
    image::Image newimg_sync(8, line_length, line_cnt, 1);
    {
        for (int line = 0; line < line_cnt - 1; line++)
        {
            int best_cor = sync.size() * 255;
            int best_pos = 0;
            for (int pos = 0; pos < line_length; pos++)
            {
                int cor = 0;
                for (int i = 0; i < sync.size(); i++)
                {
                    // float weight = abs(i - (sync.size() / 2.)) / double(sync.size() / 2.);
                    cor += abs(int((newimg.get(line * line_length + pos + i)) - sync[i]));
                }

                if (cor < best_cor)
                {
                    best_cor = cor;
                    best_pos = pos;
                }
            }

            for (int i = 0; i < sync.size(); i++)
                newimg_vs.set(line * line_length + best_pos + i, 255);

            // logger->critical("Line %d Pos %d Cor %d", line, best_pos, best_cor);
            for (int i = 0; i < line_length; i++)
            {
                int v = newimg.get(line * line_length + best_pos + i);

                float iv = ((v / 255.0) * (maxval - minval)) + minval;
                iv = (iv - minvalimg) / (maxval - minvalimg);

                v = iv * 255;

                newimg_sync.set(line * line_length + i, clamp(v));
            }
        }
    }

    image::Image newimg_color(8, 320, line_cnt, 3);

    int pre_y[320];
    int pre_ryby[320];

    //    bool color_offset = 0;

    for (int line = 0; line < line_cnt; line++)
    {
        int color_sync_length = int(0.0045 * 48000);
        double color_sync = 0;

        std::vector<double> colrsync_wip;
        for (int i = 0; i < color_sync_length; i++)
        {
            int p_t = ((10.5 + 90. + (i / double(color_sync_length - 1)) * 4.5) / 150.) * (line_length - 1);
            //  color_sync += newimg_sync.get(line * line_length + p_t) / 255.0;
            colrsync_wip.push_back(newimg_sync.get(line * line_length + p_t) / 255.0);
        }
        color_sync = get_median(colrsync_wip); // /= color_sync_length;

        bool color_sync_v = color_sync > 0.5;

        logger->trace("Color Sync %d - %d", color_sync_v, line % 2);

        for (int x = 0; x < 320; x++)
        {
            float p = x / 319.;
            int p_y = ((10.5 + p * 90.) / 150.) * (line_length - 1);
            int p_ryby = ((10.5 + 90. + 4.5 + p * 45.) / 150.) * (line_length - 1);

            int _y = newimg_sync.get(line * line_length + p_y);
            int _ryby = newimg_sync.get(line * line_length + p_ryby);

            // offset = color_sync_v ^ (line % 2);
            int ii = color_sync_v; //^(line % 2);

            if (ii == 0)
            {
                pre_y[x] = _y;
                pre_ryby[x] = _ryby;
            }
            else
            {
                int r1, g1, b1;
                int r2, g2, b2;

                //      GetColorFromYCbCr(pre_y, pre_ryby, _ryby, r1, g1, b1);
                //      GetColorFromYCbCr(_y, pre_ryby, _ryby, r2, g2, b2);

                YUV2RGB(pre_y[x], _ryby, pre_ryby[x], r1, g1, b1);
                YUV2RGB(_y, _ryby, pre_ryby[x], r2, g2, b2);

                newimg_color.set(0, x, line - 1, r1);
                newimg_color.set(1, x, line - 1, g1);
                newimg_color.set(2, x, line - 1, b1);

                newimg_color.set(0, x, line, r2);
                newimg_color.set(1, x, line, g2);
                newimg_color.set(2, x, line, b2);
            }
        }
    }

    newimg.resize_bilinear(533, newimg.height());
    image::save_img(newimg, "/tmp/test.png");

    newimg_sync.resize_bilinear(533, newimg.height());
    image::save_img(newimg_sync, "/tmp/tests.png");

    newimg_vs.resize_bilinear(533, newimg_vs.height());
    image::save_img(newimg_vs, "/tmp/testvs.png");

    image::save_img(newimg_color, "/tmp/testc.png");
}