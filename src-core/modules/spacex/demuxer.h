#pragma once

#include <vector>
#include "frame.h"

namespace spacex
{
    /*
        Simple CCSDS demuxer building CCSDS frames from CADUs. 
        VCID filtering must be done beforehand!
    */
    class Demuxer
    {
    private:
        const int MPDU_DATA_SIZE = 1191;

    private:
        SpaceXPacket currentPacket;                                                                 // Current CCSDS
        std::vector<SpaceXPacket> ccsdsBuffer;                                                      // Buffer to store what we're working on
        void pushPacket();                                                                          // We're done with it, end it
        void abortPacket();                                                                         // Abort this one!
        void readPacket(uint8_t *h);                                                                // Start a new packet
        void pushPayload(uint8_t *data, int length);                                                // Fill a packet up
        int currentPacketPayloadLength, totalPacketLength, remainingPacketLength, currentPacketEnd; // Few utils variables
        bool workingOnPacket, inHeader;                                                             // Same
       // uint8_t headerBuffer[6];                                                                    // Buffer used to buffer a header overlaping 2 CADUs
       // int inHeaderBuffer;                                                                         // Used to fill it up properly

    public:
        Demuxer();
        std::vector<SpaceXPacket> work(uint8_t *cadu); // Main function
    };
} // namespace proba