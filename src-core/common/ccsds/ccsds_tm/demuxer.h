#pragma once

#include <vector>
#include "../ccsds.h"

namespace ccsds
{
    namespace ccsds_tm
    {
        /*
        Simple CCSDS demuxer building CCSDS frames from CADUs.
        VCID filtering must be done beforehand!
    */
        class Demuxer
        {
        private:
            const int MPDU_DATA_SIZE;
            const bool HAS_INSERT_ZONE;
            const int INSERT_ZONE_SIZE;
            const int MPDU_INSERT_ZONE;

        private:
            CCSDSPacket currentCCSDSPacket;                                                             // Current CCSDS
            std::vector<CCSDSPacket> ccsdsBuffer;                                                       // Buffer to store what we're working on
            void pushPacket();                                                                          // We're done with it, end it
            void abortPacket();                                                                         // Abort this one!
            void readPacket(uint8_t *h);                                                                // Start a new packet
            void pushPayload(uint8_t *data, int length);                                                // Fill a packet up
            int currentPacketPayloadLength, totalPacketLength, remainingPacketLength, currentPacketEnd; // Few utils variables
            bool workingOnPacket, inHeader;                                                             // Same
            uint8_t headerBuffer[6];                                                                    // Buffer used to buffer a header overlaping 2 CADUs
            int inHeaderBuffer;                                                                         // Used to fill it up properly

        public:
            Demuxer(int mpdu_data_size = 884, bool hasInsertZone = false, int insertZoneSize = 2, int mpdu_insert_zone = 0);
            std::vector<CCSDSPacket> work(uint8_t *cadu); // Main function
        };
    } // namespace libccsds
} // namespace proba