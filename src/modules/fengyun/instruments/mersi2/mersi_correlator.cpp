#include "mersi_correlator.h"

MERSICorrelator::MERSICorrelator()
{
    checking = false;
    m250Frames = 0;
    m1000Frames = 0;
    complete = 0;
    incomplete = 0;
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
    image8 = reader8.getImage();
    image9 = reader9.getImage();
    image10 = reader10.getImage();
    image11 = reader11.getImage();
    image12 = reader12.getImage();
    image13 = reader13.getImage();
    image14 = reader14.getImage();
    image15 = reader15.getImage();
    image16 = reader16.getImage();
    image17 = reader17.getImage();
    image18 = reader18.getImage();
    image19 = reader19.getImage();
    image20 = reader20.getImage();
    image21 = reader21.getImage();
    image22 = reader22.getImage();
    image23 = reader23.getImage();
    image24 = reader24.getImage();
    image25 = reader25.getImage();

    // They all need to be flipped horizontally
    image1.mirror('y');
    image2.mirror('y');
    image3.mirror('y');
    image4.mirror('y');
    image5.mirror('y');
    image6.mirror('y');
    image7.mirror('y');
    image8.mirror('y');
    image9.mirror('y');
    image10.mirror('y');
    image11.mirror('y');
    image12.mirror('y');
    image13.mirror('y');
    image14.mirror('y');
    image15.mirror('y');
    image16.mirror('y');
    image17.mirror('y');
    image18.mirror('y');
    image19.mirror('y');
    image20.mirror('y');
    image21.mirror('y');
    image22.mirror('y');
    image23.mirror('y');
    image24.mirror('y');
    image25.mirror('y');
}

void MERSICorrelator::processScan()
{
    complete++;

    for (std::vector<uint8_t> &frameVec : scanBuffer)
    {
        int marker = (frameVec[3] % (int)pow(2, 3)) << 7 | frameVec[4] >> 1;

        if (marker >= 240)
        {
            m1000Frames++;

            marker -= 240;

            // Demultiplex them all!
            if (marker < 10 * 1)
                reader7.pushFrame(frameVec);
            else if (marker >= 10 * 1 && marker < 10 * 2)
                reader8.pushFrame(frameVec);
            else if (marker >= 10 * 2 && marker < 10 * 3)
                reader9.pushFrame(frameVec);
            else if (marker >= 10 * 3 && marker < 10 * 4)
                reader10.pushFrame(frameVec);
            else if (marker >= 10 * 4 && marker < 10 * 5)
                reader11.pushFrame(frameVec);
            else if (marker >= 10 * 5 && marker < 10 * 6)
                reader12.pushFrame(frameVec);
            else if (marker >= 10 * 6 && marker < 10 * 7)
                reader13.pushFrame(frameVec);
            else if (marker >= 10 * 7 && marker < 10 * 8)
                reader14.pushFrame(frameVec);
            else if (marker >= 10 * 8 && marker < 10 * 9)
                reader15.pushFrame(frameVec);
            else if (marker >= 10 * 9 && marker < 10 * 10)
                reader16.pushFrame(frameVec);
            else if (marker >= 10 * 10 && marker < 10 * 11)
                reader17.pushFrame(frameVec);
            else if (marker >= 10 * 11 && marker < 10 * 12)
                reader18.pushFrame(frameVec);
            else if (marker >= 10 * 12 && marker < 10 * 13)
                reader19.pushFrame(frameVec);
            else if (marker >= 10 * 13 && marker < 10 * 14)
                reader20.pushFrame(frameVec);
            else if (marker >= 10 * 14 && marker < 10 * 15)
                reader21.pushFrame(frameVec);
            else if (marker >= 10 * 15 && marker < 10 * 16)
                reader22.pushFrame(frameVec);
            else if (marker >= 10 * 16 && marker < 10 * 17)
                reader23.pushFrame(frameVec);
            else if (marker >= 10 * 17 && marker < 10 * 18)
                reader24.pushFrame(frameVec);
            else if (marker >= 10 * 18 && marker < 10 * 19)
                reader25.pushFrame(frameVec);
        }
        else if (marker < 240)
        {
            m250Frames++;

            // Demux those lonely 250m ones
            if (marker < 40 * 1)
                reader1.pushFrame(frameVec);
            else if (marker >= 40 * 1 && marker < 40 * 2)
                reader2.pushFrame(frameVec);
            else if (marker >= 40 * 2 && marker < 40 * 3)
                reader3.pushFrame(frameVec);
            else if (marker >= 40 * 3 && marker < 40 * 4)
                reader4.pushFrame(frameVec);
            else if (marker >= 40 * 4 && marker < 40 * 5)
                reader5.pushFrame(frameVec);
            else if (marker >= 40 * 5 && marker < 40 * 6)
                reader6.pushFrame(frameVec);
        }
    }

    scanBuffer.clear();
}

void MERSICorrelator::feedFrames(int marker, std::vector<uint8_t> &data)
{
    // Scan start
    if (marker == 0) // First frame has marker 0
    {
        checking = true;
        lastMarker = marker;
        scanBuffer.push_back(data); // Buffer it
        return;
    }

    if (checking)
    {
        if (lastMarker + 1 == marker)
        {
            lastMarker++;
            scanBuffer.push_back(data); // Buffer it

            // Complete! Let's read it.
            if (lastMarker == 429)
            {
                processScan();
            }
        }
        else // The scan is incomplete, discard
        {
            incomplete++;
            scanBuffer.clear();
            checking = false;
        }
    }
}