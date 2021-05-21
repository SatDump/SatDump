/*
 * packetfixer.h
 *
 *  Created on: 05/11/2016
 *      Author: Lucas Teske
 */

#pragma once

#include <mutex>

// // cosf(90.f * M_PI / 180.f);
// #define PacketFixer_C 0.f
// // sinf(90.f * M_PI / 180.f);
// #define PacketFixer_D 1.f

namespace sathelper
{

  enum PhaseShift
  {
    DEG_0,
    DEG_90,
    DEG_180,
    DEG_270
  };

  class PacketFixer
  {
  private:
    // static inline float S8toFloat(uint8_t v) {
    //   return ((int8_t)v) / 127.f;
    // }

    // static inline uint8_t FloatToS8(float v) {
    //   return (uint8_t) ((v + 128) * 127);
    // }

    // static inline void Rotate(uint8_t *data) {
    //   float a = S8toFloat(data[0]), b = S8toFloat(data[1]);
    //   data[0] = FloatToS8(a * PacketFixer_C - b * PacketFixer_D);
    //   data[1] = FloatToS8(a * PacketFixer_D + b * PacketFixer_C);
    // }
    static inline void Rotate(uint8_t *data)
    {
      int8_t a = data[0];
      int8_t b = data[1];
      data[0] = b;
      data[1] = ~a;
    }

  public:
    void fixPacket(uint8_t *data, uint32_t length, PhaseShift phaseShift, bool iqInversion);
  };
} // namespace sathelper