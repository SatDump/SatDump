#pragma once

#include "dvbs2.h"
#include "common/dsp/demod/constellation.h"

namespace dvbs2
{
    struct dvb_cgf_holder
    {
        int frame_slot_count;
        dvbs2::dvbs2_constellation_t constellation;
        dsp::constellation_type_t constel_obj_type;
        dvbs2::dvbs2_framesize_t framesize;
        dvbs2::dvbs2_code_rate_t coderate;
        bool pilots;
        float g1, g2;
    };

    dvb_cgf_holder get_dvbs2_cfg(int modcod, bool shortframes, bool pilots)
    {
        dvb_cgf_holder cfg;

        cfg.pilots = pilots;

        if (modcod <= 0)
            throw std::runtime_error("MODCOD cannot be <= 0!");
        else if (modcod < 12) // QPSK Modcods
        {
            cfg.frame_slot_count = shortframes ? 90 : 360;
            cfg.constellation = dvbs2::MOD_QPSK;
            cfg.constel_obj_type = dsp::QPSK;

            if (modcod == 1)
                cfg.coderate = dvbs2::C1_4;
            else if (modcod == 2)
                cfg.coderate = dvbs2::C1_3;
            else if (modcod == 3)
                cfg.coderate = dvbs2::C2_5;
            else if (modcod == 4)
                cfg.coderate = dvbs2::C1_2;
            else if (modcod == 5)
                cfg.coderate = dvbs2::C3_5;
            else if (modcod == 6)
                cfg.coderate = dvbs2::C2_3;
            else if (modcod == 7)
                cfg.coderate = dvbs2::C3_4;
            else if (modcod == 8)
                cfg.coderate = dvbs2::C4_5;
            else if (modcod == 9)
                cfg.coderate = dvbs2::C5_6;
            else if (modcod == 10)
                cfg.coderate = dvbs2::C8_9;
            else if (modcod == 11)
                cfg.coderate = dvbs2::C9_10;
        }
        else if (modcod < 18) // 8-PSK Modcods
        {
            cfg.frame_slot_count = shortframes ? 60 : 240;
            cfg.constellation = dvbs2::MOD_8PSK;
            cfg.constel_obj_type = dsp::PSK8;

            if (modcod == 12)
                cfg.coderate = dvbs2::C3_5;
            else if (modcod == 13)
                cfg.coderate = dvbs2::C2_3;
            else if (modcod == 14)
                cfg.coderate = dvbs2::C3_4;
            else if (modcod == 15)
                cfg.coderate = dvbs2::C5_6;
            else if (modcod == 16)
                cfg.coderate = dvbs2::C8_9;
            else if (modcod == 17)
                cfg.coderate = dvbs2::C9_10;
        }
        else if (modcod < 24) // 16-APSK Modcods
        {
            cfg.frame_slot_count = shortframes ? 45 : 180;
            cfg.constellation = dvbs2::MOD_16APSK;
            cfg.constel_obj_type = dsp::APSK16;

            if (modcod == 18)
            {
                cfg.coderate = dvbs2::C2_3;
                cfg.g1 = 3.15;
            }
            else if (modcod == 19)
            {
                cfg.coderate = dvbs2::C3_4;
                cfg.g1 = 2.85;
            }
            else if (modcod == 20)
            {
                cfg.coderate = dvbs2::C4_5;
                cfg.g1 = 2.75;
            }
            else if (modcod == 21)
            {
                cfg.coderate = dvbs2::C5_6;
                cfg.g1 = 2.70;
            }
            else if (modcod == 22)
            {
                cfg.coderate = dvbs2::C8_9;
                cfg.g1 = 2.60;
            }
            else if (modcod == 23)
            {
                cfg.coderate = dvbs2::C9_10;
                cfg.g1 = 2.57;
            }
        }
        else if (modcod < 29) // 32-APSK Modcods
        {
            cfg.frame_slot_count = shortframes ? 36 : 144;
            cfg.constellation = dvbs2::MOD_32APSK;
            cfg.constel_obj_type = dsp::APSK32;

            if (modcod == 24)
            {
                cfg.coderate = dvbs2::C3_4;
                cfg.g1 = 2.84;
                cfg.g2 = 5.27;
            }
            else if (modcod == 25)
            {
                cfg.coderate = dvbs2::C4_5;
                cfg.g1 = 2.72;
                cfg.g2 = 4.87;
            }
            else if (modcod == 26)
            {
                cfg.coderate = dvbs2::C5_6;
                cfg.g1 = 2.64;
                cfg.g2 = 4.64;
            }
            else if (modcod == 27)
            {
                cfg.coderate = dvbs2::C8_9;
                cfg.g1 = 2.54;
                cfg.g2 = 4.33;
            }
            else if (modcod == 28)
            {
                cfg.coderate = dvbs2::C9_10;
                cfg.g1 = 2.53;
                cfg.g2 = 4.30;
            }
        }
        else
            throw std::runtime_error("MODCOD not (yet?) supported!");

        cfg.framesize = shortframes ? dvbs2::FECFRAME_SHORT : dvbs2::FECFRAME_NORMAL;

        return cfg;
    }
}