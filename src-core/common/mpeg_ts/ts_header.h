#pragma once

#include <cstdint>

namespace mpeg_ts
{
    struct TSHeader
    {
        uint8_t sync; // Sync byte, should be 0x47 (G)
        bool tei;     // Transport Error Indicator
        bool pusi;    // Payload Unit Start Indicator
        bool tp;      // Transport Priority
        uint16_t pid; // Packet Identifier
        uint8_t tsc;  // Transport Scrambling Control
        uint8_t afc;  // Adapatation Field Control
        uint8_t cont; // Continuity Counter

        void parse(uint8_t *ts);
    };
};