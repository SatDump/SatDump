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

#include "common/dsp/filter/firdes.h"
#include "common/image/image.h"
#include "common/image/io.h"
#include "common/utils.h"
#include "core/exception.h"
#include "dsp/base/dsp_buffer.h"
#include "dsp/block.h"
#include "dsp/filter/fir.h"
#include "dsp/utils/hilbert.h"
#include "dsp/utils/quadrature_demod.h"
#include "logger.h"
#include <complex.h>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <thread>
#include <vector>

class SSTVSync
{
public:
    float samplerate = 48000;
    float line_time = 0.150;
    float sync_time = 0.0105;

    int line_length;
    int sync_length;

    int sync_line_buffer_len;
    float *sync_line_buffer;

    std::vector<float> sync;

    float calcValFromFreq(float freq) { return (2. * M_PI * freq) / samplerate; }

    int found_offset = -1;
    int skip = -1;

    std::vector<float> full_image;

    std::vector<uint8_t> ir, ig, ib;

    const float minval = calcValFromFreq(1200);
    const float minvalimg = calcValFromFreq(1488); // TODOREWORK should be 1500?
    const float maxval = calcValFromFreq(2300);

    float lineGetScaledIMG(float v)
    {
        v = (v - minvalimg) / (maxval - minvalimg);

        if (v < 0)
            v = 0;
        if (v > 1)
            v = 1;
        return v;
    }

    // COLOR
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

public:
    SSTVSync()
    {
        line_length = round(samplerate * line_time);
        sync_length = round(samplerate * sync_time);

        sync_line_buffer_len = line_length * 2; //+ sync_length;
        sync_line_buffer = new float[sync_line_buffer_len];
        memset(sync_line_buffer, 0, sync_line_buffer_len * sizeof(float));

        for (int i = 0; i < sync_length; i++)
            sync.push_back(calcValFromFreq(1200));
    }

    int pre_y[320];
    int pre_ryby[320];

    void lineProc(std::vector<float> line)
    {
        full_image.insert(full_image.end(), line.begin(), line.end());

        {
            int color_sync_length = int(0.0045 * 48000);
            double color_sync = 0;

            std::vector<double> colrsync_wip;
            for (int i = 0; i < color_sync_length; i++)
            {
                int p_t = ((10.5 + 90. + (i / double(color_sync_length - 1)) * 4.5) / 150.) * (line_length - 1);
                //  color_sync += newimg_sync.get(line * line_length + p_t) / 255.0;
                colrsync_wip.push_back(lineGetScaledIMG(line[p_t]));
            }
            color_sync = get_median(colrsync_wip); // /= color_sync_length;

            bool color_sync_v = color_sync > 0.5;

            logger->trace("Color Sync %d", color_sync_v);

            std::vector<uint8_t> lr(320 * 2), lg(320 * 2), lb(320 * 2);

            for (int x = 0; x < 320; x++)
            {
                float p = x / 319.;
                int p_y = ((10.5 + p * 90.) / 150.) * (line_length - 1);
                int p_ryby = ((10.5 + 90. + 4.5 + p * 45.) / 150.) * (line_length - 1);

                int _y = lineGetScaledIMG(line[p_y]) * 255;       // newimg_sync.get(line * line_length + p_y);
                int _ryby = lineGetScaledIMG(line[p_ryby]) * 255; // newimg_sync.get(line * line_length + p_ryby);

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

                    lr[320 * 0 + x] = r1;
                    lg[320 * 0 + x] = g1;
                    lb[320 * 0 + x] = b1;

                    lr[320 * 1 + x] = r2;
                    lg[320 * 1 + x] = g2;
                    lb[320 * 1 + x] = b2;
                }
            }

            if (color_sync_v)
            {
                ir.insert(ir.end(), lr.begin(), lr.end());
                ig.insert(ig.end(), lg.begin(), lg.end());
                ib.insert(ib.end(), lb.begin(), lb.end());
            }
        }
    }

    void saveImg()
    {
        std::vector<uint8_t> imgvec;

        for (auto &v : full_image)
        {
            v = (v - minvalimg) / (maxval - minvalimg);

            if (v < 0)
                v = 0;
            if (v > 1)
                v = 1;
            imgvec.push_back(v * 255);
        }

        int lines = full_image.size() / line_length;
        image::Image img(imgvec.data(), 8, line_length, lines, 1);

        img.resize_bilinear(533, img.height());
        image::save_img(img, "/tmp/testnewsync.png");

        {
            std::vector<uint8_t> imgvec2;
            imgvec2.insert(imgvec2.end(), ir.begin(), ir.end());
            imgvec2.insert(imgvec2.end(), ig.begin(), ig.end());
            imgvec2.insert(imgvec2.end(), ib.begin(), ib.end());

            image::Image img(imgvec2.data(), 8, 320, ir.size() / 320, 3);
            image::save_img(img, "/tmp/testnewsync_COLOR.png");
        }
    }

    void push(float *buf, int len)
    {
        for (int is = 0; is < len; is++)
        {
            memmove(&sync_line_buffer[0], &sync_line_buffer[1], (sync_line_buffer_len - 1) * sizeof(float));
            sync_line_buffer[sync_line_buffer_len - 1] = buf[is];

#if 1
            if (skip-- > 0)
                continue;

            {
                int best_cor = sync.size() * 255;
                int best_pos = 0;
                for (int pos = 0; pos < line_length; pos++)
                {
                    int cor = 0;
                    for (int i = 0; i < sync.size(); i++)
                    {
                        // float weight = abs(i - (sync.size() / 2.)) / double(sync.size() / 2.);

                        float fv = (double)sync_line_buffer[pos + i];
                        fv = (fv - minval) / (maxval - minval);

                        if (fv < 0)
                            fv = 0;
                        if (fv > 1)
                            fv = 1;

                        cor += abs(int(fv * 255) - sync[i]);
                    }

                    if (cor < best_cor)
                    {
                        best_cor = cor;
                        best_pos = pos;
                    }
                }

                lineProc(std::vector<float>(sync_line_buffer + best_pos, sync_line_buffer + best_pos + line_length));
                skip = line_length;
            }
#endif

#if 0
            if (skip-- > 0)
                continue;

            if (found_offset >= 0)
            {
                if (found_offset == 0)
                {
                    full_image.insert(full_image.end(), sync_line_buffer, sync_line_buffer + line_length);
                    skip = sync_length;
                }
                found_offset--;
            }
            else
            {
                float best_cor = sync.size() * 255;
                int best_pos = 0;
                for (int pos = 0; pos < line_length; pos++)
                {
                    float cor = 0;
                    for (int i = 0; i < sync.size(); i++)
                    {
                        // float weight = abs(i - (sync.size() / 2.)) / double(sync.size() / 2.);
                        cor += fabs(sync_line_buffer[pos + i] - sync[i]);
                    }

                    if (cor < best_cor)
                    {
                        best_cor = cor;
                        best_pos = pos;
                    }
                }

                found_offset = best_pos;
            }
#endif
        }
    }
};

int main(int argc, char *argv[])
{
    initLogger();

    std::ifstream input_float("/home/alan/Downloads/SSTV/sstv_out.f32");
    std::ofstream output_float("/home/alan/Downloads/SSTV/sstv_out_cdemod.f32");

    satdump::ndsp::FIRBlock<float> bpf;
    bpf.set_cfg("taps", dsp::firdes::band_pass(1, 48000, 1200, 2300, 500, dsp::fft::window::WIN_KAISER));
    satdump::ndsp::HilbertBlock hilb;
    satdump::ndsp::QuadratureDemodBlock quad;

    satdump::ndsp::BlockIO io_in;
    io_in.name = "in";
    io_in.type = satdump::ndsp::DSP_SAMPLE_TYPE_F32;
    io_in.fifo = std::make_shared<satdump::ndsp::DspBufferFifo>(4);

    bpf.set_input(io_in, 0);
    hilb.link(&bpf, 0, 0, 4);
    quad.link(&hilb, 0, 0, 4);

    bpf.start();
    hilb.start();
    quad.start();

    auto io_out = quad.get_output(0, 4);

    SSTVSync sstv;

    std::thread readThread(
        [&]()
        {
            while (true)
            {
                satdump::ndsp::DSPBuffer iblk;
                io_out.fifo->wait_dequeue(iblk);

                if (iblk.isTerminator())
                {
                    iblk.free();
                    break;
                }

                //                logger->info("Got Something!");

                output_float.write(((char *)iblk.getSamples<float>()), iblk.size * sizeof(float));
                sstv.push(iblk.getSamples<float>(), iblk.size);

                iblk.free();
            }

            logger->info("BYE!");
        });

    while (!input_float.eof())
    {
        auto nblk = satdump::ndsp::DSPBuffer::newBufferSamples<float>(8192);
        input_float.read(((char *)nblk.getSamples<float>()), 8192 * sizeof(float));
        nblk.size = 8192;
        io_in.fifo->wait_enqueue(nblk);
        logger->trace(input_float.tellg());
    }

    logger->info("STOP");
    io_in.fifo->wait_enqueue(satdump::ndsp::DSPBuffer::newBufferTerminator());

    bpf.stop();
    hilb.stop();
    quad.stop();

    if (readThread.joinable())
        readThread.join();

    sstv.saveImg();
}
