#include "demuxer.h"
#include <vector>
#include "header.h"
#include <cstring>

namespace spacex
{
    Demuxer::Demuxer()

    {
        // Init variables
        currentPacket.length = 0;
        currentPacketPayloadLength = 0;
        remainingPacketLength = 0;
        totalPacketLength = 0;
        inHeader = false;
        abortPacket();
    }

    // Fill header and packet lengths
    void Demuxer::readPacket(uint8_t *h)
    {
        workingOnPacket = true;
        currentPacket.length = ((h[0] & 0b00000111) << 8 | h[1]);
        currentPacketPayloadLength = currentPacket.length;
        totalPacketLength = currentPacketPayloadLength + 2;
        remainingPacketLength = currentPacketPayloadLength + 2;
    }

    // Clear everything and push into packet buffer
    void Demuxer::pushPacket()
    {
        if (currentPacket.payload.size() > 2)
            ccsdsBuffer.push_back(currentPacket);

        currentPacket.payload.clear();
        // currentPacket.length = 0;
        // currentPacketPayloadLength = 0;
        remainingPacketLength = 0;
        workingOnPacket = false;
    }

    // Push data into the packet
    void Demuxer::pushPayload(uint8_t *data, int length)
    {
        for (int i = 0; i < length; i++)
            currentPacket.payload.push_back(data[i]);

        remainingPacketLength -= length;
    }

    // ABORT!
    void Demuxer::abortPacket()
    {
        workingOnPacket = false;
        currentPacket.payload.clear();
        currentPacket.length = 0;
        currentPacketPayloadLength = 0;
        remainingPacketLength = 0;
    };

    std::vector<SpaceXPacket> Demuxer::work(uint8_t *cadu)
    {
        ccsdsBuffer.clear(); // Clear buffer from previous run

        SpaceXHeader mpdu = parseHeader(cadu); // Parse M-PDU Header

        //logger->trace(mpdu.first_header_pointer);

        // Sanity check, if the first header point points outside the data payload
        if (mpdu.first_header_pointer >= MPDU_DATA_SIZE)
        {
            return ccsdsBuffer;
        }

        // Maybe?
        //if (mpdu.first_header_pointer == 0)
        //    return ccsdsBuffer;

        // Are we working on a packet right now? Still got stuff to write? Then do it!
        if (remainingPacketLength > 0 && workingOnPacket)
        {
            // If there's a header, it takes priority
            if (mpdu.first_header_pointer < 2047)
            {
                // Finish filling up that packet
                int toWrite = (remainingPacketLength + 0) > mpdu.first_header_pointer + 1 ? (mpdu.first_header_pointer + 1) - 0 : remainingPacketLength;
                pushPayload(&mpdu.data[0], toWrite);
                remainingPacketLength = 0;
            }
            else
            {
                // Write what we have
                int toWrite = (remainingPacketLength + 0) > MPDU_DATA_SIZE - 0 ? MPDU_DATA_SIZE - 0 : remainingPacketLength;
                pushPayload(&mpdu.data[0], toWrite);
            }
        }

        // Are we done writing a packet? Push it
        if (remainingPacketLength <= 0 && workingOnPacket)
        {
            pushPacket();
        }

        // Is there a new header here?
        if (mpdu.first_header_pointer < 2047)
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
                    pushPayload(&mpdu.data[mpdu.first_header_pointer], currentPacketPayloadLength);
                    pushPacket();
                }
                else
                {
                    abortPacket();
                }

                // Compute next possible header
                int nextHeaderPointer = mpdu.first_header_pointer + totalPacketLength;

                //logger->trace(nextHeaderPointer);

                while (nextHeaderPointer < MPDU_DATA_SIZE)
                {
                    // The header fits!
                    if (nextHeaderPointer < MPDU_DATA_SIZE)
                    {
                        readPacket(&mpdu.data[nextHeaderPointer]);

                        int toWrite = remainingPacketLength > (MPDU_DATA_SIZE - (nextHeaderPointer)) ? (MPDU_DATA_SIZE - (nextHeaderPointer)) : remainingPacketLength;
                        pushPayload(&mpdu.data[nextHeaderPointer], toWrite);
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
                    int toWrite = remainingPacketLength > (MPDU_DATA_SIZE - (mpdu.first_header_pointer)) ? (MPDU_DATA_SIZE - (mpdu.first_header_pointer)) : remainingPacketLength;
                    pushPayload(&mpdu.data[mpdu.first_header_pointer], toWrite);
                }
            }
        }

        return ccsdsBuffer;
    }
} // namespace proba