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

#include "common/dsp/complex.h"

int main(int argc, char *argv[])
{
    initLogger();

#if 0
    std::ifstream input_frm(argv[1]);
    std::ofstream output_frm("test_mdl.frm");

    uint8_t cadu[4640];

    while (!input_frm.eof())
    {
        input_frm.read((char *)cadu, 464);

        int marker = (cadu[3] & 0b111) << 3 | (cadu[4] >> 5);

        logger->critical(marker);

        if (marker != 44)
            continue;

        // logger->info(cadu[marker * 30]);
        // if (cadu[marker * 30] == 168)
        output_frm.write((char *)cadu, 464);
    }
#endif

#if 0
    std::ifstream input_frm(argv[1]);
    std::ofstream output_frm(argv[2]);

    complex_t iq_buffer[8192];
    int8_t buffer_mag_phase[8192 * 2];

    while (!input_frm.eof())
    {
        input_frm.read((char *)iq_buffer, 8192 * sizeof(complex_t));

        for (int i = 0; i < 8192; i++)
        {
            complex_t &v = iq_buffer[i];

            float mag = v.norm();
            float phase = atan2f(v.imag, v.real);

            mag = (mag * 127) / 2.0;
            phase = (phase * 127) / M_PI;

            if (mag > 127)
                mag = 1227;
            if (mag < -127)
                mag = -127;

            buffer_mag_phase[i * 2 + 0] = mag;
            buffer_mag_phase[i * 2 + 1] = phase;
        }

        output_frm.write((char *)buffer_mag_phase, 8192 * 2);
    }
#endif

#if 0
    std::ifstream input_frm(argv[1]);
    std::ofstream output_frm(argv[2]);

    complex_t iq_buffer[8192];
    int8_t buffer_mag_phase[8192 * 2];

    while (!input_frm.eof())
    {
        input_frm.read((char *)buffer_mag_phase, 8192 * 2);

        for (int i = 0; i < 8192; i++)
        {
            float mag = buffer_mag_phase[i * 2 + 0];
            float phase = buffer_mag_phase[i * 2 + 1];

            mag = (mag / 127) * 2.0;
            phase = (phase / 127) * M_PI;

            iq_buffer[i].real = sinf(phase) * mag;
            iq_buffer[i].real = cosf(phase) * mag;
        }

        output_frm.write((char *)iq_buffer, 8192 * sizeof(complex_t));
    }
#endif
}
