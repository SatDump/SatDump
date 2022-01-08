#include "deframer.h"

// Definitely still needs tuning
#define THRESOLD_STATE_3 12
#define THRESOLD_STATE_2 2
#define THRESOLD_STATE_1 6
#define THRESOLD_STATE_0 0

// Returns the asked bit!
template <typename T>
inline bool getBit(T &data, int &bit)
{
    return (data >> bit) & 1;
}

namespace ccsds
{
    namespace ccsds_1_0_1024
    {
        // Compare 2 32-bits values bit per bit
        int checkSyncMarker(uint32_t &marker, uint32_t totest)
        {
            int errors = 0;
            for (int i = 31; i >= 0; i--)
            {
                bool markerBit, testBit;
                markerBit = getBit<uint32_t>(marker, i);
                testBit = getBit<uint32_t>(totest, i);
                if (markerBit != testBit)
                    errors++;
            }
            return errors;
        }

        CADUDeframer::CADUDeframer(bool derand)
        {
            d_rantab[0] = 0;
            d_rantab[1] = 0;
            d_rantab[2] = 0;
            d_rantab[3] = 0;
            if (derand)
            {
                // From gr-poes-weather (including comments!)
                unsigned char feedbk, randm = 0xff;
                // Original Polynomial is :  1 + x3 + x5 + x7 +x8
                for (int i = 4; i < CADU_SIZE; i++)
                { //4ASM bytes + 1020bytes = 32 + 8160 bits in CADU packet

                    d_rantab[i] = 0;
                    for (int j = 0; j <= 7; j++)
                    {
                        d_rantab[i] = d_rantab[i] << 1;
                        if (randm & 0x80) //80h = 1000 0000b
                            d_rantab[i]++;

                        //Bit-Wise AND between: Fixed shift register(95h) and the state of the
                        // feedback register: randm
                        feedbk = randm & 0x95; //95h = 1001 0101--> bits 1,3,5,8
                        //feedback contains the contents of the registers masked by the polynomial
                        //  1 + x3 + x5 + xt +x8 = 95 h
                        randm = randm << 1;

                        if ((((feedbk & 0x80) ^ (0x80 & feedbk << 3)) ^ (0x80 & (feedbk << 5))) ^ (0x80 & (feedbk << 7)))
                            randm++;
                    }
                }
            }
            else
            {
                for (int i = 4; i < CADU_SIZE; i++)
                    d_rantab[i] = 0;
            }

            // Default values
            writeFrame = false;
            wroteBits = 8;
            wroteBytes = 0;
            skip = 0;
            good = 0;
            errors = 0;
            state = THRESOLD_STATE_0;
            bit_inversion = false;
        }

        int CADUDeframer::getState()
        {
            return state;
        }

        void CADUDeframer::pushBit(uint8_t bit2)
        {
            // Get a bit, perform bit inversion if necessary
            uint8_t bit = bit_inversion ? !bit2 : bit2;
            // Push it into out shifter
            shifter = (shifter << 1) | bit;

            // Are we writing a frame?
            if (writeFrame)
            {
                // First loop : add clean ASM Marker
                if (wroteBytes == 0)
                {
                    frameBuffer[0] = CADU_ASM_1;
                    frameBuffer[1] = CADU_ASM_2;
                    frameBuffer[2] = CADU_ASM_3;
                    frameBuffer[3] = CADU_ASM_4;
                    wroteBytes += 4;
                }

                // Push bit into out own 1-byte shifter
                outBuffer = (outBuffer << 1) | bit;

                // If we filled the buffer, output it
                if (--wroteBits == 0)
                {
                    frameBuffer[wroteBytes] = outBuffer ^ d_rantab[wroteBytes];
                    wroteBytes++;
                    wroteBits = 8;
                }

                // Did we write the entire frame?
                if (wroteBytes == CADU_SIZE)
                {
                    // Exit of this loop, reset values and push the frame
                    writeFrame = false;
                    wroteBits = 8;
                    wroteBytes = 0;
                    skip = CADU_ASM_SIZE * 8; // Push back next ASM in shifter
                    frames.push_back(frameBuffer);
                }

                return;
            }

            // Skip a few run if necessary
            if (skip > 1)
            {
                skip--;
                return;
            }
            else if (skip == 1) // Last run should NOT reset the loop
                skip--;

            // State 0 : Searches bit-per-bit for a perfect sync marker. If one is found, we jump to state 6!
            if (state == THRESOLD_STATE_0)
            {
                if (shifter == CADU_ASM)
                {
                    //numFrames++;
                    writeFrame = true;
                    state = THRESOLD_STATE_1;
                    bit_inversion = false;
                    errors = 0;
                    sep_errors = 0;
                    good = 0;
                }
                else if (shifter == CADU_ASM_INV)
                {
                    //numFrames++;
                    writeFrame = true;
                    state = THRESOLD_STATE_1;
                    bit_inversion = true;
                    errors = 0;
                    sep_errors = 0;
                    good = 0;
                }
            }
            // State 1 : Each header is expect 1024 bytes away. Only 6 mistmatches tolerated.
            // If 5 consecutive good frames are found, we hop to state 22, though, 5 consecutive
            // errors (here's why errors is reset each time a frame is good) means reset to state 0
            // 2 frame errors pushes us to state 2
            else if (state == THRESOLD_STATE_1)
            {
                if (checkSyncMarker(shifter, CADU_ASM) <= state)
                {
                    //numFrames++;
                    writeFrame = true;
                    good++;
                    errors = 0;

                    if (good == 5)
                    {
                        state = THRESOLD_STATE_3;
                        good = 0;
                        errors = 0;
                    }
                }
                else
                {
                    errors++;
                    sep_errors++;

                    if (errors == 5)
                    {
                        state = THRESOLD_STATE_0;
                        bit_inversion = false;
                        //skip = 1;
                        errors = 0;
                        sep_errors = 0;
                        good = 0;
                    }
                    else if (sep_errors == 2)
                    {
                        state = THRESOLD_STATE_2;
                        state_2_bits_count = 0;
                        //bitsToIncrement = 1;
                        errors = 0;
                        sep_errors = 0;
                        good = 0;
                    }
                }
            }
            // State 2 : Goes back to bit-per-bit syncing... 3 frame scanned and we got back to state 0, 1 good and back to 6!
            else if (state == THRESOLD_STATE_2)
            {
                if (checkSyncMarker(shifter, CADU_ASM) <= state)
                {
                    //numFrames++;
                    writeFrame = true;
                    state = THRESOLD_STATE_1;
                    //skip = 1024 * 8;
                    errors = 0;
                    sep_errors = 0;
                    good = 0;
                }
                else
                {
                    state_2_bits_count++;
                    errors++;
                    //skip = CADU_SIZE * 8;

                    if (state_2_bits_count >= 5 * 1024 * 8)
                    {
                        state = THRESOLD_STATE_0;
                        bit_inversion = false;
                        //bitsToIncrement = 1;
                        errors = 0;
                        sep_errors = 0;
                        good = 0;
                    }
                }
            }
            // State 3 : We assume perfect lock and allow very high mismatchs.
            // 1 error and back to state 6
            // Note : Lowering the thresold seems to yield better of a sync
            else if (state == THRESOLD_STATE_3)
            {
                if (checkSyncMarker(shifter, CADU_ASM) <= state)
                {
                    //numFrames++;
                    writeFrame = true;
                }
                else
                {
                    errors = 0;
                    good = 0;
                    sep_errors = 0;
                    state = THRESOLD_STATE_1;
                }
            }
        }

        std::vector<std::array<uint8_t, CADU_SIZE>> CADUDeframer::work(uint8_t *input, int size)
        {
            frames.clear();

            // Loop in all bytes
            for (int byteInBuf = 0; byteInBuf < size; byteInBuf++)
            {
                // Loop in all bits!
                for (int i = 7; i >= 0; i--)
                {
                    pushBit(getBit<uint8_t>(input[byteInBuf], i));
                }
            }

            // Output what we found if anything
            return frames;
        }

        std::vector<std::array<uint8_t, CADU_SIZE>> CADUDeframer::work_bits(uint8_t *input, int size)
        {
            frames.clear();

            // Loop in all bytes
            for (int byteInBuf = 0; byteInBuf < size; byteInBuf++)
            {
                pushBit(input[byteInBuf]);
            }

            // Output what we found if anything
            return frames;
        }
    }
}