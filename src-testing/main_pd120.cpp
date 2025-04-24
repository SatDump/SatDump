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
#include "logger.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>

std::vector<uint8_t> genSeq()
{
    std::vector<uint8_t> t;
    for (int i = 0; i < 960; i++)
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

    std::ifstream data_in("/home/alan/Downloads/SSTV/sstv_out.f32");

    float minval = calcValFromFreq(1200);
    float minvalimg = calcValFromFreq(1488); // TODO should be 1500?
    float maxval = calcValFromFreq(2300);

    std::vector<uint16_t> wip_img;
    while (!data_in.eof())
    {
        float v;
        data_in.read((char *)&v, sizeof(float));

        v = (v - minval) / (maxval - minval);

        if (v < 0)
            v = 0;
        if (v > 1)
            v = 1;
        wip_img.push_back(v * 65535);
    }

    int line_length = 0.50848 * 48000; //+ 100;
    int line_cnt = wip_img.size() / line_length;

    image::Image newimg(wip_img.data(), 16, line_length, line_cnt, 1);
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
                    cor += abs(int(((newimg.get(line * line_length + pos + i)) >> 8) - sync[i]));
                }

                if (cor < best_cor)
                {
                    best_cor = cor;
                    best_pos = pos;
                }
            }

            for (int i = 0; i < sync.size(); i++)
                newimg_vs.set(line * line_length + best_pos + i, 65535);

            // logger->critical("Line %d Pos %d Cor %d", line, best_pos, best_cor);
            for (int i = 0; i < line_length; i++)
            {
                int v = newimg.get(line * line_length + best_pos + i);

                float iv = ((v / 65535.0) * (maxval - minval)) + minval;
                iv = (iv - minvalimg) / (maxval - minvalimg);

                v = iv * 255;

                newimg_sync.set(line * line_length + i, clamp(v));
            }
        }
    }

    const double lwidth = 640;
    image::Image newimg_color(8, lwidth, line_cnt * 2, 3);

    for (int line = 0; line < line_cnt; line++)
    {
        for (int x = 0; x < lwidth; x++)
        {
            float p = x / (lwidth - 1.);
            int p_y1 = ((0.020 + 0.00208 + 0.1216 * 0 + p * 0.1216) / 0.50848) * (line_length - 1);
            int p_v = ((0.020 + 0.00208 + 0.1216 * 1 + p * 0.1216) / 0.50848) * (line_length - 1);
            int p_u = ((0.020 + 0.00208 + 0.1216 * 2 + p * 0.1216) / 0.50848) * (line_length - 1);
            int p_y2 = ((0.020 + 0.00208 + 0.1216 * 3 + p * 0.1216) / 0.50848) * (line_length - 1);

            int _y1 = newimg_sync.get(line * line_length + p_y1);
            int _u = newimg_sync.get(line * line_length + p_u);
            int _v = newimg_sync.get(line * line_length + p_v);
            int _y2 = newimg_sync.get(line * line_length + p_y2);

            {
                int r1, g1, b1;
                int r2, g2, b2;

                GetColorFromYCbCr(_y1, _u, _v, r1, g1, b1);
                GetColorFromYCbCr(_y2, _u, _v, r2, g2, b2);

                // YUV2RGB(_y1, _u, _v, r1, g1, b1);
                // YUV2RGB(_y2, _u, _v, r2, g2, b2);

                newimg_color.set(0, x, line * 2 + 0, r1);
                newimg_color.set(1, x, line * 2 + 0, g1);
                newimg_color.set(2, x, line * 2 + 0, b1);

                newimg_color.set(0, x, line * 2 + 1, r2);
                newimg_color.set(1, x, line * 2 + 1, g2);
                newimg_color.set(2, x, line * 2 + 1, b2);
            }
        }
    }

    // newimg.resize_bilinear(2560, newimg.height());
    image::save_img(newimg, "/tmp/test.png");

    //  newimg_sync.resize_bilinear(2560, newimg.height());
    image::save_img(newimg_sync, "/tmp/tests.png");

    // newimg_vs.resize_bilinear(2560, newimg_vs.height());
    image::save_img(newimg_vs, "/tmp/testvs.png");

    image::save_img(newimg_color, "/tmp/testc.png");
}