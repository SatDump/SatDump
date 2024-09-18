#include "wbd_decoder.h"

namespace cluster
{
    std::vector<MajorFrame> WBDdecoder::work(uint8_t *frm)
    {
        std::vector<MajorFrame> outputframes;

        if (frm[3] % 4 == 0)
        {
            if (wbdwip.payload.size() == 1090 * 4 - 8)
                outputframes.push_back(wbdwip);
            wbdwip = MajorFrame();
        }
        else if (frm[3] % 4 == 1)
        {
            wbdwip.antennaselect = (frm[5] >> 4) & 0b11; // 0 Ez, 1 Bz, 2 By, 3 Ey
            wbdwip.convfreq = (frm[5] >> 2) & 0b11;      // conversion freq, 0, 125.454, 250.908, 501.816k
            wbdwip.hgagc = frm[5] & 0b11;                // Higher AGC select
            wbdwip.commands = frm[5] >> 5 & 0b1;         // Command Status (0=No cmds; 1=Cmds received)
            wbdwip.OBDH = frm[5] >> 6 & 0b1;             // OBDH Interface (0=Primary; 1=Redundant)
            wbdwip.ADpower = frm[5] >> 4 & 0b1;          // seems to be always on
            wbdwip.VCXO = frm[5] >> 7;                   // locked unlocked, 0 is locked
        }
        else if (frm[3] % 4 == 2)
        {

            wbdwip.AGC = frm[4] >> 5 & 0b1;              // AGC MAN/AUTO 0 for Auto, 1 for Manual
            wbdwip.gainlevel = frm[4] >> 1 & 0b1111;     // Gain, 16steps as per 3.2.2.4-1
            wbdwip.antennaselect = (frm[5] >> 4) & 0b11; // 0 Ez, 1 Bz, 2 By, 3 Ey
            wbdwip.convfreq = (frm[5] >> 2) & 0b11;      // conversion freq, 0, 125.454, 250.908, 501.816k
            wbdwip.hgagc = frm[5] & 0b11;                // Higher AGC select
        }
        else if (frm[3] % 4 == 3)
        {
            wbdwip.commands = frm[5] >> 5 & 0b1;     // Command Status (0=No cmds; 1=Cmds received)
            wbdwip.OBDH = frm[5] >> 6 & 0b1;         // OBDH Interface (0=Primary; 1=Redundant)
            wbdwip.ADpower = frm[5] >> 4 & 0b1;      // seems to be always on
            wbdwip.VCXO = frm[5] >> 7;               // locked unlocked, 0 is locked
            wbdwip.outputmode = frm[4] >> 2 & 0b111; // sampling rate, bit depth, duty cycle as per 3.2.2.5-1
            wbdwip.lwagc = frm[4] & 0b11;            // Lower AGC select
        }

        for (int i = 6; i < 1096; i++)
            if (i != 88 && i != 89)
                wbdwip.payload.push_back(frm[i]);

        return outputframes;
    }
}