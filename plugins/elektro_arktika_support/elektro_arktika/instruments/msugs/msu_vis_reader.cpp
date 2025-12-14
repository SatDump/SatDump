#include "msu_vis_reader.h"
#include "logger.h"
#include <string>

namespace elektro_arktika
{
    namespace msugs
    {
        MSUVISReader::MSUVISReader()
        {
            imageBuffer1 = new unsigned short[17200 * 6004];
            imageBuffer2 = new unsigned short[17200 * 6004];
            timestamps.resize(17200, -1);
            frames = 0;
        }

        MSUVISReader::~MSUVISReader()
        {
            delete[] imageBuffer1;
            delete[] imageBuffer2;
        }

        void MSUVISReader::pushFrame(uint8_t *data, bool apply_correction)
        {

            // First bit is never set, mask it
            int counter = (data[8] << 8 | data[9]) & 0x7fff;

            // Does correction logic if specified by the user
            if (apply_correction)
            {
                // Unlocks if we are fstarting a new image
                // Not implemented in RDAS!
                /*
                if (counter_locked) && (counter == 1 || counter == 2)) {
                    counter_locked = false;

                } else*/

                if (!counter_locked)
                {
                    if (counter == global_counter + 1)
                    {
                        // LOCKED!
                        logger->debug("Counter correction LOCKED! Counter: " + std::to_string(counter));
                        counter_locked = true;
                    }
                    else
                    {
                        // We can't lock, save this counter for a check on the next one
                        global_counter = counter;
                    }
                }
                if (counter_locked)
                {
                    // Makes sure dropped frames don't throw us off, one/two line skips are fine
                    // Corrector therefore doesn't fix two LSB flips, but this improves reliability
                    // with projections and such
                    if (abs(global_counter - counter) > 2)
                    {
                        counter = global_counter + 1;
                    }

                    global_counter = counter;
                }
            }
            // Warning: The above code MUST have an unlock in the FD end! Since the code doesn't have
            // handling for more than one FD at a time right now, I didn't add this. Just keep it in mind!

            // Sanity check
            if (counter >= 17200)
                return;

            // Offset to start reading from
            int pos = 5 * 38;

            // Convert to 10 bits values
            for (int i = 0; i < 12044; i += 4)
            {
                msuLineBuffer[i] = (data[pos + 0] << 2) | (data[pos + 1] >> 6);
                msuLineBuffer[i + 1] = ((data[pos + 1] % 64) << 4) | (data[pos + 2] >> 4);
                msuLineBuffer[i + 2] = ((data[pos + 2] % 16) << 6) | (data[pos + 3] >> 2);
                msuLineBuffer[i + 3] = ((data[pos + 3] % 4) << 8) | data[pos + 4];
                pos += 5;
            }

            // Deinterleave and load into our image buffer
            for (int i = 0; i < 6004; i++)
            {
                imageBuffer1[counter * 6004 + i] = msuLineBuffer[i * 2 + 0] << 6;
                imageBuffer2[counter * 6004 + i] = msuLineBuffer[i * 2 + 1] << 6;
            }

            uint64_t data_time = data[10] << 32 | data[11] << 24 | data[12] << 16 | data[13] << 8 | data[14];
            //  data_time = 0;
            double timestamp = data_time / 256.0; //.56570155902006;
            timestamp += 1735204808.2837029;
            timestamp -= 1800 + 88;
            timestamps[counter] = timestamp;

            frames++;
        }

        image::Image MSUVISReader::getImage1() { return image::Image(&imageBuffer1[0], 16, 6004, 17200, 1); }

        image::Image MSUVISReader::getImage2() { return image::Image(&imageBuffer2[0], 16, 6004, 17200, 1); }
    } // namespace msugs
} // namespace elektro_arktika