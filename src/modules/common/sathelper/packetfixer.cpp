/*
 * packetfixer.cpp
 *
 *  Created on: 05/11/2016
 *      Author: Lucas Teske
 */

#include "packetfixer.h"

namespace sathelper
{
  void PacketFixer::fixPacket(uint8_t *data, uint32_t length, PhaseShift phaseShift, bool iqInversion)
  {
    if (iqInversion || phaseShift != PhaseShift::DEG_0)
    {
      for (uint32_t i = 0; i < length; i += 2)
      {
        if (iqInversion)
        {
          uint8_t x = data[i + 1];
          data[i + 1] = data[i];
          data[i] = x;
        }
        if (phaseShift == PhaseShift::DEG_90 || phaseShift == PhaseShift::DEG_270)
        {
          Rotate(&data[i]);
        }
        if (phaseShift == PhaseShift::DEG_180 || phaseShift == PhaseShift::DEG_270)
        {
          data[i] ^= 0xFF;
          data[i + 1] ^= 0xFF;
        }
      }
    }
  }
} // namespace sathelper