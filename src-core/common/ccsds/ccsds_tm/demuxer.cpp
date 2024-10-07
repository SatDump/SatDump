#include "demuxer.h"
#include <vector>
#include "mpdu.h"
#include <cstring>

#define HEADER_LENGTH 6

namespace ccsds
{
    namespace ccsds_tm
    {
        Demuxer::Demuxer(int mpdu_data_size, bool hasInsertZone, int insertZoneSize, int mpdu_insert_zone) : MPDU_DATA_SIZE(mpdu_data_size),
                                                                                                             HAS_INSERT_ZONE(hasInsertZone),
                                                                                                             INSERT_ZONE_SIZE(insertZoneSize),
                                                                                                             MPDU_INSERT_ZONE(mpdu_insert_zone)
        {
            // Init variables
            currentCCSDSPacket.header.packet_length = 0;
            currentPacketPayloadLength = 0;
            remainingPacketLength = 0;
            totalPacketLength = 0;
            inHeader = false;
            inHeaderBuffer = 0;
            abortPacket();
        }

        // Fill header and packet lengths
        void Demuxer::readPacket(uint8_t *h)
        {
            workingOnPacket = true;
            currentCCSDSPacket.header = parseCCSDSHeader(h);
            currentPacketPayloadLength = currentCCSDSPacket.header.packet_length + 1;
            totalPacketLength = currentPacketPayloadLength + HEADER_LENGTH;
            remainingPacketLength = currentPacketPayloadLength;
        }

        // Clear everything and push into packet buffer
        void Demuxer::pushPacket()
        {
            ccsdsBuffer.push_back(currentCCSDSPacket);
            currentCCSDSPacket.payload.clear();
            currentCCSDSPacket.header.packet_length = 0;
            currentPacketPayloadLength = 0;
            remainingPacketLength = 0;
            workingOnPacket = false;
        }

        // Push data into the packet
        void Demuxer::pushPayload(uint8_t *data, int length)
        {
            for (int i = 0; i < length; i++)
                currentCCSDSPacket.payload.push_back(data[i]);

            remainingPacketLength -= length;
        }

        // ABORT!
        void Demuxer::abortPacket()
        {
            workingOnPacket = false;
            currentCCSDSPacket.payload.clear();
            currentCCSDSPacket.header.packet_length = 0;
            currentPacketPayloadLength = 0;
            remainingPacketLength = 0;
        };

        std::vector<CCSDSPacket> Demuxer::work(uint8_t *cadu)
        {
            ccsdsBuffer.clear(); // Clear buffer from previous run

            MPDU mpdu = parseMPDU(cadu, HAS_INSERT_ZONE, INSERT_ZONE_SIZE, MPDU_INSERT_ZONE); // Parse M-PDU Header

            // Sanity check, if the first header point points outside the data payload
            if (mpdu.first_header_pointer < 2047 && mpdu.first_header_pointer >= MPDU_DATA_SIZE)
            {
                return ccsdsBuffer;
            }

            // This is idle
            if (mpdu.first_header_pointer == 2046)
            {
                return ccsdsBuffer;
            }

            // If we're in a header or so,
            // we may need to read with an offset
            int offset = 0;

            // We're parsing a header!
            if (inHeader)
            {
                inHeader = false;

                // Copy the remaining data into our header
                std::memcpy(&headerBuffer[inHeaderBuffer], &mpdu.data[0], 6 - inHeaderBuffer);
                offset = 6 - inHeaderBuffer;
                inHeaderBuffer += 6 - inHeaderBuffer;

                // Parse it
                readPacket(headerBuffer);
            }

            // Are we working on a packet right now? Still got stuff to write? Then do it!
            if (remainingPacketLength > 0 && workingOnPacket)
            {
                // If there's a header, it takes priority
                if (mpdu.first_header_pointer < 2047)
                {
                    // Finish filling up that packet
                    int toWrite = (remainingPacketLength + offset) > mpdu.first_header_pointer + 1 ? (mpdu.first_header_pointer + 1) - offset : remainingPacketLength;
                    pushPayload(&mpdu.data[offset], toWrite);
                    remainingPacketLength = 0;
                }
                else
                {
                    // Write what we have
                    int toWrite = (remainingPacketLength + offset) > MPDU_DATA_SIZE - offset ? MPDU_DATA_SIZE - offset : remainingPacketLength;
                    pushPayload(&mpdu.data[offset], toWrite);
                }
            }

            // Are we done writing a packet? Push it
            if (remainingPacketLength == 0 && workingOnPacket)
            {
                pushPacket();
            }

            // Is there a new header here?
            if (mpdu.first_header_pointer < 2047)
            {
                // The full header fits
                if (mpdu.first_header_pointer + HEADER_LENGTH < MPDU_DATA_SIZE)
                {
                    // Parse it
                    readPacket(&mpdu.data[mpdu.first_header_pointer]);

                    // Compute possible secondary headers in that packet
                    bool hasSecondHeader = MPDU_DATA_SIZE > mpdu.first_header_pointer + totalPacketLength;

                    // A second header can fit, so search for it!
                    if (hasSecondHeader)
                    {
                        // Write first packet
                        if (mpdu.first_header_pointer + totalPacketLength < MPDU_DATA_SIZE)
                        {
                            pushPayload(&mpdu.data[mpdu.first_header_pointer + 6], currentPacketPayloadLength);
                            pushPacket();
                        }
                        else
                        {
                            abortPacket();
                        }

                        // Compute next possible header
                        int nextHeaderPointer = mpdu.first_header_pointer + totalPacketLength;

                        while (nextHeaderPointer < MPDU_DATA_SIZE)
                        {
                            // The header fits!
                            if (nextHeaderPointer + HEADER_LENGTH < MPDU_DATA_SIZE)
                            {
                                readPacket(&mpdu.data[nextHeaderPointer]);

                                int toWrite = remainingPacketLength > (MPDU_DATA_SIZE - (nextHeaderPointer + 6)) ? (MPDU_DATA_SIZE - (nextHeaderPointer + 6)) : remainingPacketLength;
                                pushPayload(&mpdu.data[nextHeaderPointer + 6], toWrite);
                            }
                            // Only part of the header fits. At least 1 byte has to be there or it'll be in the next frame
                            else if (nextHeaderPointer < MPDU_DATA_SIZE)
                            {
                                inHeader = true;
                                inHeaderBuffer = 0;
                                std::memcpy(headerBuffer, &mpdu.data[nextHeaderPointer], MPDU_DATA_SIZE - nextHeaderPointer);
                                inHeaderBuffer += MPDU_DATA_SIZE - nextHeaderPointer;
                                break;
                            }

                            // That packet is done? Write it
                            if (remainingPacketLength == 0 && workingOnPacket)
                            {
                                pushPacket();
                            }

                            // Update to next pointer
                            nextHeaderPointer = nextHeaderPointer + totalPacketLength;
                        }
                    }
                    else
                    {
                        // Sanity check
                        if (workingOnPacket)
                        {
                            // Fill that packet
                            int toWrite = remainingPacketLength > (MPDU_DATA_SIZE - (mpdu.first_header_pointer + 6)) ? (MPDU_DATA_SIZE - (mpdu.first_header_pointer + 6)) : remainingPacketLength;
                            pushPayload(&mpdu.data[mpdu.first_header_pointer + 6], toWrite);
                        }
                    }
                }
                else if (mpdu.first_header_pointer < MPDU_DATA_SIZE)
                {
                    // The header doesn't fit, so fill what we have
                    inHeader = true;
                    inHeaderBuffer = 0;
                    std::memcpy(headerBuffer, &mpdu.data[mpdu.first_header_pointer], MPDU_DATA_SIZE - mpdu.first_header_pointer);
                    inHeaderBuffer += MPDU_DATA_SIZE - mpdu.first_header_pointer;
                }
            }

            return ccsdsBuffer;
        } // namespace libccsds
    }     // namespace libccsds
} // namespace proba