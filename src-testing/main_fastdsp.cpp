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

#include "common/dsp/buffer.h"
#include "common/dsp/filter/firdes.h"
#include "dsp/agc/agc.h"
#include "dsp/base/dsp_buffer.h"
#include "dsp/clock_recovery/clock_recovery_mm.h"
#include "dsp/device/dev.h"
#include "dsp/filter/rrc.h"
#include "dsp/io/file_source.h"
#include "dsp/pll/costas.h"
#include "init.h"
#include "logger.h"
#include <complex.h>
#include <cstdint>
#include <fstream>
#include <volk/volk_malloc.h>

int main(int argc, char *argv[])
{
    initLogger();

    logger->set_level(slog::LOG_OFF);
    satdump::initSatdump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    double samplerate = 61.44e6;
    double symbolrate = 30.72e6;

#if 0
    // Init bladeRF
    auto devs = satdump::ndsp::getDeviceList(satdump::ndsp::DeviceBlock::MODE_SINGLE_RX);

    std::shared_ptr<satdump::ndsp::DeviceBlock> dev;
    for (auto &d : devs)
    {
        if (d.type == "bladerf")
        {
            dev = satdump::ndsp::getDeviceInstanceFromInfo(d, satdump::ndsp::DeviceBlock::MODE_SINGLE_RX);
            break;
        }
    }

    dev->setStreamSamplerate(0, false, samplerate);
    dev->setStreamFrequency(0, false, 1000e6);
    dev->set_cfg("rx1_gain", 10);
#else
    auto dev = std::make_shared<satdump::ndsp::FileSourceBlock>();

    dev->set_cfg("file", "/home/alan/Downloads/13-28-38_61440000SPS_700000000Hz.s8");
    dev->set_cfg("type", "cs8");
    dev->set_cfg("buffer_size", 8192 * 2);
#endif

    // Blocks
    auto agc = std::make_shared<satdump::ndsp::AGCBlock<complex_t>>();

    auto rrc = std::make_shared<satdump::ndsp::FIRBlock<complex_t>>();
    rrc->set_cfg("taps", dsp::firdes::root_raised_cosine(1, samplerate, symbolrate, 0.5, 31));
    rrc->set_cfg("buffer_size", 1e6);

    auto rec = std::make_shared<satdump::ndsp::MMClockRecoveryBlock<complex_t>>();
    rec->set_cfg("omega", double(samplerate) / double(symbolrate));

    auto costas = std::make_shared<satdump::ndsp::CostasBlock>();
    costas->set_cfg("order", 2);

    // Link
    agc->link(dev.get(), 0, 0, 100);
    rrc->link(agc.get(), 0, 0, 4);
    rec->link(rrc.get(), 0, 0, 4);
    costas->link(rec.get(), 0, 0, 4);
    auto final_out = costas->get_output(0, 4);

    // Start
    dev->start();
    agc->start();
    rrc->start();
    rec->start();
    costas->start();

    std::ofstream data_out("/home/alan/Downloads/testl2live.soft", std::ios::binary);

    while (1)
    {
        satdump::ndsp::DSPBuffer iblk;
        final_out.fifo->wait_dequeue(iblk);

        if (iblk.isTerminator())
        {
            iblk.free();
            break;
        }

        int nsamples = iblk.size;
        auto samples = iblk.getSamples<complex_t>();

        int8_t *tmp_buf = dsp::create_volk_buffer<int8_t>(nsamples);

        for (int i = 0; i < nsamples; i++)
        {
#if 1
            float v = samples[i].real * 50;
            if (v > 127)
                v = 127;
            if (v < -127)
                v = -127;
            tmp_buf[i] = v;
#else
            float v1 = samples[i].real * 50;
            float v2 = samples[i].imag * 50;
            if (v1 > 127)
                v1 = 127;
            if (v1 < -127)
                v1 = -127;
            if (v2 > 127)
                v2 = 127;
            if (v2 < -127)
                v2 = -127;
            tmp_buf[i * 2 + 0] = v1;
            tmp_buf[i * 2 + 1] = v2;
#endif
        }

        data_out.write((char *)tmp_buf, nsamples);

        volk_free(tmp_buf);

        iblk.free();
    }
}
