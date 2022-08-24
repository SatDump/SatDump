/*
 * Copyright (C) 2013 by Miroslav Slugen <thunder.m@email.cz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gain.h"

int mirisdr_set_gain(mirisdr_dev_t *p)
{
    uint32_t reg1 = 1, reg6 = 6;
#if MIRISDR_DEBUG >= 1
    fprintf (stderr, "gain reduction baseband: %d, lna: %d, mixer: %d\n", p->gain_reduction_baseband, p->gain_reduction_lna, p->gain_reduction_mixer);
#endif
// Reset to 0xf380 to enable gain control added Dec 5 2014 SM5BSZ
//    mirisdr_write_reg(p, 0x08, 0xf380);

    /* Receiver Gain Control */
    /* 0-3 => registr */
    /* 4-9 => baseband, 0 - 59, 60-63 je stejné jako 59 */
    /* 10-11 => mixer gain reduction pouze pro AM režim */
    /* 12 => mixer gain reduction -19dB */
    /* 13 => lna gain reduction -24dB */
    /* 14-16 => DC kalibrace */
    /* 17 => zrychlená DC kalibrace */
    reg1 |= p->gain_reduction_baseband << 4;

    // Mixbuffer is on AM1 and AM2 inputs only
    if (p->band == MIRISDR_BAND_AM1)
    {
        reg1 |= (p->gain_reduction_mixbuffer & 0x03) << 10;
    }
    else if (p->band == MIRISDR_BAND_AM2)
    {
        fprintf(stderr, "mirisdr_set_gain: gain_reduction_mixbuffer: %d\n", p->gain_reduction_mixbuffer);
        reg1 |= (p->gain_reduction_mixbuffer == 0 ? 0x0 : 0x03) << 10;
    }
    else
    {
        reg1 |= 0x0 << 10;
    }

    reg1 |= p->gain_reduction_mixer << 12;

    // LNA is not on AM1 nor AM2 inputs
    if ((p->band == MIRISDR_BAND_AM1) || (p->band == MIRISDR_BAND_AM2))
    {
        reg1 |= 0x0 << 13;
    }
    else
    {
        reg1 |= p->gain_reduction_lna << 13;
    }

    reg1 |= MIRISDR_DC_OFFSET_CALIBRATION_PERIODIC2 << 14;
    reg1 |= MIRISDR_DC_OFFSET_CALIBRATION_SPEEDUP_OFF << 17;
    mirisdr_write_reg(p, 0x09, reg1);

    /* DC Offset Calibration setup */
    reg6 |= 0x1F << 4;
    reg6 |= 0x800 << 10;
    mirisdr_write_reg(p, 0x09, reg6);
//// set to 0xf300 to select AM input added Dec 5 2014 SM5BSZ
//    if (p->freq < 50000000)
//      {
//      mirisdr_write_reg(p, 0x08, 0xf300);
//      }
//    else
//      {
//      if (p->freq >= 108000000)
//        {
//// Nothing between 00 and 0xff helps to switch in signals above 108 MHz.
////        mirisdr_write_reg(p, 0x08, 0xf3ff);
//        }
//      }

    return 0;
}

int mirisdr_get_tuner_gains(mirisdr_dev_t *dev, int *gains)
{
    int i;
    (void) dev;
    i = 103;
    if (gains)
    {
        for (i = 0; i <= 102; i++)
        {
            gains[i] = i;
        }
    }

    return i;
}

int mirisdr_set_tuner_gain(mirisdr_dev_t *p, int gain)
{
    p->gain = gain;
    /*
     * Pro VHF režim je lna zapnutý +24dB, mixer +19dB a baseband
     * je možné nastavovat plynule od 0 - 59 dB, z toho je maximální
     * zesílení 102 dB
     *
     * For VHF mode LNA is turned on to + 24 db, mixer to + 19 dB and baseband
     * can be adjusted continuously from 0 to 59 db, of which the maximum gain of 102 db
     */
    if (p->gain > 102)
    {
        p->gain = 102;
    }
    else if (p->gain < 0)
    {
        goto gain_auto;
    }

    /* Nejvyšší citlivost vždy bez redukce mixeru a lna */
    /* Always the highest sensitivity without reducing the mixer and LNA */
    if (p->gain >= 43)
    {
        p->gain_reduction_lna = 0;
        p->gain_reduction_mixbuffer = 0; // LNA equivalent for AM inputs
        p->gain_reduction_mixer = 0;
        p->gain_reduction_baseband = 59 - (p->gain - 43);
    }
    else if (p->gain >= 19)
    {
        p->gain_reduction_lna = 1;
        p->gain_reduction_mixbuffer = 3; // LNA equivalent for AM inputs (AM1: 18dB / AM2: 24 dB)
        p->gain_reduction_mixer = 0;
        p->gain_reduction_baseband = 59 - (p->gain - 19);
    }
    else
    {
        p->gain_reduction_lna = 1;
        p->gain_reduction_mixbuffer = 3; // LNA equivalent for AM inputs (AM1: 18dB / AM2: 24 dB)
        p->gain_reduction_mixer = 1;
        p->gain_reduction_baseband = 59 - p->gain;
    }

    return mirisdr_set_gain(p);

    gain_auto: return mirisdr_set_tuner_gain_mode(p, 0);
}

int mirisdr_get_tuner_gain(mirisdr_dev_t *p)
{
    int gain = 0;

    if (p->gain < 0)
        goto gain_auto;

    gain += 59 - p->gain_reduction_baseband;

    if (p->band == MIRISDR_BAND_AM1)
    {
        gain += 18 - 6*p->gain_reduction_mixbuffer;
    }
    else if (p->band == MIRISDR_BAND_AM2)
    {
        gain += (p->gain_reduction_mixbuffer == 0 ? 24 : 0);
    }
    else
    {
        if (!p->gain_reduction_lna) {
            gain += 24;
        }
    }

    if (!p->gain_reduction_mixer) {
        gain += 19;
    }

    return gain;

    gain_auto: return -1;
}

int mirisdr_set_tuner_gain_mode(mirisdr_dev_t *p, int mode)
{
//    if (!mode) {
//        p->gain = -1;
//#if MIRISDR_DEBUG >= 1
//        fprintf( stderr, "gain mode: auto\n");
//#endif
//        mirisdr_write_reg(p, 0x09, 0x014281);
//        mirisdr_write_reg(p, 0x09, 0x3FFFF6);
//    } else if (p->gain < 0) {
//#if MIRISDR_DEBUG >= 1
//        fprintf( stderr, "gain mode: manual\n");
//#endif
//        p->gain = DEFAULT_GAIN;
//    }

    return 0;
}

int mirisdr_get_tuner_gain_mode(mirisdr_dev_t *p)
{
    return (p->gain < 0) ? 0 : 1;
}

/*
 * Gain reduction is an index that depends on the AM mode (only applies to AM inputs)
 *          AM1     AM2
 * 0x00    0 dB    0 dB
 * 0x01    6 dB   24 dB
 * 0x10   12 dB   24 dB
 * 0x11   18 dB   24 dB
 */
int mirisdr_set_mixer_gain(mirisdr_dev_t *p, int gain)
{
    p->gain_reduction_mixer = gain;

    return mirisdr_set_gain(p);
}

int mirisdr_set_mixbuffer_gain(mirisdr_dev_t *p, int gain)
{
    p->gain_reduction_mixbuffer = gain & 0x03;

    return mirisdr_set_gain(p);
}

int mirisdr_set_lna_gain(mirisdr_dev_t *p, int gain)
{
    p->gain_reduction_lna = gain;

    return mirisdr_set_gain(p);
}

int mirisdr_set_baseband_gain(mirisdr_dev_t *p, int gain)
{
    p->gain_reduction_baseband = 59 - gain;

    return mirisdr_set_gain(p);
}

int mirisdr_get_mixer_gain(mirisdr_dev_t *p)
{
    return p->gain_reduction_mixer ? 0 : 19;
}

int mirisdr_get_mixbuffer_gain(mirisdr_dev_t *p)
{
    if (p->band == MIRISDR_BAND_AM1)
    {
        return 18 - 6*p->gain_reduction_mixbuffer;
    }
    else if (p->band == MIRISDR_BAND_AM2)
    {
        return p->gain_reduction_mixbuffer == 0 ? 24 : 0;
    }
    else
    {
        return 0;
    }
}

int mirisdr_get_lna_gain(mirisdr_dev_t *p)
{
    return p->gain_reduction_lna ? 0 : 24;
}

int mirisdr_get_baseband_gain(mirisdr_dev_t *p)
{
    return 59 - p->gain_reduction_baseband;
}

