#include "mersi_correlator.h"

namespace fengyun
{
    namespace mersill
    {
        MERSICorrelator::MERSICorrelator()
        {
            m250Frames = 0;
            m1000Frames = 0;
            complete = 0;

            for (int i = 0; i < 130; i++)
                scanBuffer[i] = std::vector<uint8_t>(99120 / 8);
        }

        // Export decoded data
        void MERSICorrelator::makeImages()
        {
            image1 = reader1.getImage();
            image2 = reader2.getImage();
            image3 = reader3.getImage();
            image4 = reader4.getImage();
            image5 = reader5.getImage();
            image6 = reader6.getImage();
            image7 = reader7.getImage();

            // They all need to be flipped horizontally
            image1.mirror('y');
            image2.mirror('y');
            image3.mirror('y');
            image4.mirror('y');
            image5.mirror('y');
            image6.mirror('y');
            image7.mirror('y');
        }

        void MERSICorrelator::processScan()
        {
            complete++;

            for (int i = 0; i < 130; i++)
            {
                int marker = i; // Silly, but cleans things up
                std::vector<uint8_t> &frameVec = scanBuffer[i];

                if (marker >= 80)
                {
                    m1000Frames++;

                    marker -= 80;

                    // Demultiplex them all!
                    if (marker < 10 * 1)
                        reader3.pushFrame(frameVec);
                    else if (marker >= 10 * 1 && marker < 10 * 2)
                        reader4.pushFrame(frameVec);
                    else if (marker >= 10 * 2 && marker < 10 * 3)
                        reader5.pushFrame(frameVec);
                    else if (marker >= 10 * 3 && marker < 10 * 4)
                        reader6.pushFrame(frameVec);
                    else if (marker >= 10 * 4 && marker < 10 * 5)
                        reader7.pushFrame(frameVec);
                }
                else if (marker < 80)
                {
                    m250Frames++;

                    // Demux those lonely 250m ones
                    if (marker < 40 * 1)
                        reader1.pushFrame(frameVec);
                    else if (marker >= 40 * 1 && marker < 40 * 2)
                        reader2.pushFrame(frameVec);
                }
            }

            for (int i = 0; i < 130; i++)
                scanBuffer[i] = std::vector<uint8_t>(99120 / 8);
        }

        void MERSICorrelator::feedFrames(int marker, std::vector<uint8_t> &data)
        {
            // Much simpler than before!
            if (marker == 0)
                processScan();

            if (marker < 130)
                scanBuffer[marker] = data;
        }
    } // namespace mersi2
} // namespace fengyun