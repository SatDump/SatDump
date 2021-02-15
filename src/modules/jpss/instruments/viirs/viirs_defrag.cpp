#include "viirs_defrag.h"

namespace jpss
{
    namespace viirs
    {
        // And I present... The worse correction code I've written to this date! Will by all means need a rewrite at some point,
        // but it does OK for now, I guess?
        void defragChannel(cimg_library::CImg<unsigned short> &image, Channel &channelSettings)
        {
            unsigned short *imageData = image.data();
            int imageWidth = channelSettings.totalWidth;
            int segmentHeight = channelSettings.zoneHeight;
            int segmentCount = image.height() / segmentHeight;

            // Correction for moderate channels
            if (channelSettings.zoneHeight == 15)
            {
                for (int i = 0; i < segmentCount; i++)
                {
                    // Correct for detectors 1 and 4
                    int det4pos = channelSettings.zoneWidth[0] + channelSettings.zoneWidth[1] + channelSettings.zoneWidth[2] + channelSettings.zoneWidth[3]; // Just to avoid a long line
                    std::memcpy(&imageData[(i * segmentHeight * imageWidth) + channelSettings.zoneWidth[0]], &imageData[((i * segmentHeight + 1) * imageWidth) + channelSettings.zoneWidth[0]], channelSettings.zoneWidth[1] * 2);
                    std::memcpy(&imageData[(i * segmentHeight * imageWidth) + det4pos], &imageData[((i * segmentHeight + 1) * imageWidth) + det4pos], channelSettings.zoneWidth[4] * 2);

                    // Correct for detector 0 and 5
                    int det5pos = channelSettings.zoneWidth[0] + channelSettings.zoneWidth[1] + channelSettings.zoneWidth[2] + channelSettings.zoneWidth[3] + channelSettings.zoneWidth[4]; // Just to avoid a long line

                    int startPos = channelSettings.totalWidth == 4064 ? 2 : 4;
                    int arraySize = channelSettings.totalWidth == 4064 ? 12 : 10;

                    // Stretch it to fill the black parts
                    unsigned short heightSegBuffer[arraySize];
                    for (int y = 0; y < channelSettings.zoneWidth[0]; y++)
                    {
                        for (int u = 0; u < arraySize; u++)
                            heightSegBuffer[u] = imageData[((i * segmentHeight + startPos + u) * imageWidth) + y];

                        for (int u = 0; u < 15; u++)
                            imageData[((i * segmentHeight + u) * imageWidth) + y] = heightSegBuffer[(int)(u * (arraySize / 15.0f))];
                    }
                    for (int y = 0; y < channelSettings.zoneWidth[5]; y++)
                    {
                        for (int u = 0; u < arraySize; u++)
                            heightSegBuffer[u] = imageData[((i * segmentHeight + startPos + u) * imageWidth) + det5pos + y];

                        for (int u = 0; u < 15; u++)
                            imageData[((i * segmentHeight + u) * imageWidth) + det5pos + y] = heightSegBuffer[(int)(u * (arraySize / 15.0f))];
                    }
                }
            }
            // Imaging bands
            else if (channelSettings.zoneHeight == 31)
            {
                for (int i = 0; i < segmentCount; i++)
                {
                    // Correct for detectors 1 and 4
                    int det4pos = channelSettings.zoneWidth[0] + channelSettings.zoneWidth[1] + channelSettings.zoneWidth[2] + channelSettings.zoneWidth[3]; // Just to avoid a long line

                    int startPos = 2;
                    int arraySize = 28;

                    // Stretch it to fill the black parts
                    unsigned short heightSegBuffer[arraySize];
                    for (int y = 0; y < channelSettings.zoneWidth[1]; y++)
                    {
                        for (int u = 0; u < arraySize; u++)
                            heightSegBuffer[u] = imageData[((i * segmentHeight + startPos + u) * imageWidth + channelSettings.zoneWidth[0]) + y];

                        for (int u = 0; u < 31; u++)
                            imageData[((i * segmentHeight + u) * imageWidth + channelSettings.zoneWidth[0]) + y] = heightSegBuffer[(int)(u * (arraySize / 31.0f))];
                    }
                    for (int y = 0; y < channelSettings.zoneWidth[4]; y++)
                    {
                        for (int u = 0; u < arraySize; u++)
                            heightSegBuffer[u] = imageData[((i * segmentHeight + startPos + u) * imageWidth) + det4pos + y];

                        for (int u = 0; u < 31; u++)
                            imageData[((i * segmentHeight + u) * imageWidth) + det4pos + y] = heightSegBuffer[(int)(u * (arraySize / 31.0f))];
                    }

                    // Correct for detector 0 and 5
                    int det5pos = channelSettings.zoneWidth[0] + channelSettings.zoneWidth[1] + channelSettings.zoneWidth[2] + channelSettings.zoneWidth[3] + channelSettings.zoneWidth[4]; // Just to avoid a long line

                    startPos = 4;
                    arraySize = 24;

                    // Stretch it to fill the black parts
                    for (int y = 0; y < channelSettings.zoneWidth[0]; y++)
                    {
                        for (int u = 0; u < arraySize; u++)
                            heightSegBuffer[u] = imageData[((i * segmentHeight + startPos + u) * imageWidth) + y];

                        for (int u = 0; u < 31; u++)
                            imageData[((i * segmentHeight + u) * imageWidth) + y] = heightSegBuffer[(int)(u * (arraySize / 31.0f))];
                    }
                    for (int y = 0; y < channelSettings.zoneWidth[5]; y++)
                    {
                        for (int u = 0; u < arraySize; u++)
                            heightSegBuffer[u] = imageData[((i * segmentHeight + startPos + u) * imageWidth) + det5pos + y];

                        for (int u = 0; u < 31; u++)
                            imageData[((i * segmentHeight + u) * imageWidth) + det5pos + y] = heightSegBuffer[(int)(u * (arraySize / 31.0f))];
                    }
                }
            }
        }
    } // namespace viirs
} // namespace jpss