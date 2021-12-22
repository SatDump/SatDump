#include "channel_reader.h"
#include "common/ccsds/ccsds_time.h"
extern "C"
{
#include <stdlib.h>
#include <stdint.h>
#include "libs/aec/libaec.h"
#include <string.h>
}

namespace jpss
{
    namespace viirs
    {
        // Forward-declare to clean things up
        int uses_decompress(char *input, char *output, int inputLen, int outputLen);

        VIIRSReader::VIIRSReader(Channel &ch)
        {
            channelSettings = ch;
            foundData = false;
            currentSegment = 0;
            imageBuffer = std::shared_ptr<unsigned short>(new unsigned short[20000 * channelSettings.totalWidth], [](unsigned short *p)
                                                          { delete[] p; });
            lines = 0;
        }

        VIIRSReader::~VIIRSReader()
        {
        }

        void VIIRSReader::feed(ccsds::CCSDSPacket &packet)
        {
            if (packet.header.apid != channelSettings.apid)
                return;

            if (packet.header.sequence_flag == 1)
            {
                // Start a new segment
                HeaderPacket header(packet);

                Segment segment(header);
                endSequenceCount = header.header.packet_sequence_count + header.number_of_packets + 2;
                segments.push_back(segment);
                currentSegment++;

                foundData = true;
            }
            else if (foundData && packet.header.packet_sequence_count <= endSequenceCount)
            {
                // Parse its contents
                BodyPacket body(packet);

                if (body.detector <= channelSettings.zoneHeight)
                {
                    if (!body.empty)
                    {
                        for (int i = 0; i < 6; i++)
                        {
                            Detector &detector = body.detectors[i];

                            if (detector.data_payload_size > 8 && (detector.sync_word == body.sync_word_pattern || detector.sync_word == 0xC000FFEE))
                            {
                                // Init decompressed data buffer
                                detector.initDataPayload(channelSettings.zoneWidth[i], channelSettings.oversampleZone[i]);

                                // Decompress
                                uses_decompress((char *)detector.data_payload, (char *)detector.decompressedPayload, detector.data_payload_size, channelSettings.zoneWidth[i] * channelSettings.oversampleZone[i] * 2);

                                // We read as big-endian, now convert to little endian for this to work...
                                for (int y = 0; y < channelSettings.zoneWidth[i] * channelSettings.oversampleZone[i]; y++)
                                {
#ifdef _WIN32
                                    body.detectors[i].decompressedPayload[y] = ((body.detectors[i].decompressedPayload[y] & 0xff) << 8) | ((body.detectors[i].decompressedPayload[y] & 0xff00) >> 8);
#else
                                    body.detectors[i].decompressedPayload[y] = __builtin_bswap16(body.detectors[i].decompressedPayload[y]);
#endif
                                }

                                // If this detector is oversampled, decimate and average samples
                                if (channelSettings.oversampleZone[i] > 1)
                                {
                                    uint16_t val = 0;

                                    for (int y = 0; y < channelSettings.zoneWidth[i] * channelSettings.oversampleZone[i]; y += channelSettings.oversampleZone[i])
                                    {
                                        if (channelSettings.oversampleZone[i] == 2)

                                            val = (body.detectors[i].decompressedPayload[y] + body.detectors[i].decompressedPayload[y + 1]) / channelSettings.oversampleZone[i];
                                        else if (channelSettings.oversampleZone[i] == 3)
                                            val = (body.detectors[i].decompressedPayload[y] + body.detectors[i].decompressedPayload[y + 1] + body.detectors[i].decompressedPayload[y + 2]) / channelSettings.oversampleZone[i];

                                        body.detectors[i].decompressedPayload[y / channelSettings.oversampleZone[i]] = val;
                                    }
                                }
                            }
                        }
                    }

                    segments[currentSegment - 1].body[body.detector] = body;
                }
            }
        }

        void VIIRSReader::differentialDecode(VIIRSReader &channelSource, int deci)
        {
            // Differential decoding between 2 channels
            for (int segment_number = 0; segment_number < std::min<int>(channelSource.segments.size(), segments.size()); segment_number++)
            {
                Segment &segInit = channelSource.segments[segment_number];
                Segment &segCurrent = segments[segment_number];

                for (int body_number = 0; body_number < 32; body_number++)
                {
                    BodyPacket &bodyInit = segInit.body[body_number / deci];
                    BodyPacket &bodyCurrent = segCurrent.body[body_number];

                    if (!bodyInit.empty && !bodyCurrent.empty)
                    {
                        for (int i = 0; i < 6; i++)
                        {
                            Detector &detectInit = bodyInit.detectors[i];
                            Detector &detectCurrent = bodyCurrent.detectors[i];

                            // Still check we're working with good data
                            if (detectCurrent.data_payload_size > 8 && (detectCurrent.sync_word == bodyCurrent.sync_word_pattern || detectCurrent.sync_word == 0xC000FFEE) && detectInit.data_payload_size > 8 && (detectInit.sync_word == bodyInit.sync_word_pattern || detectInit.sync_word == 0xC000FFEE))
                            {
                                for (int y = 0; y < channelSettings.zoneWidth[i] * channelSettings.oversampleZone[i]; y++)
                                    detectCurrent.decompressedPayload[y] = (int)(detectCurrent.decompressedPayload[y] + detectInit.decompressedPayload[y / deci]) - (int)16383;
                            }
                        }
                    }
                }
            }
        }

        image::Image<uint16_t> VIIRSReader::getImage()
        {
            timestamps.clear();

            for (int i = segments.size() - 1; i >= 0; i--)
            {
                Segment &seg = segments[i];

                for (int y = 0; y < channelSettings.zoneHeight; y++)
                {
                    BodyPacket &body = seg.body[y];

                    if (!body.empty)
                    {
                        int offset = 0;

                        for (int det = 0; det < 6; det++)
                        {
                            // Check if this detector is valid
                            if (body.detectors[det].data_payload_size > 8 && (body.detectors[det].sync_word == body.sync_word_pattern || body.detectors[det].sync_word == 0xC000FFEE))
                            {
                                std::memcpy(&imageBuffer.get()[lines * channelSettings.totalWidth + offset], body.detectors[det].decompressedPayload, channelSettings.zoneWidth[det] * 2);
                            }
                            else
                            {
                                // It's not? Fill it with zeros
                                std::fill(&imageBuffer.get()[lines * channelSettings.totalWidth + offset], &imageBuffer.get()[lines * channelSettings.totalWidth + offset + channelSettings.zoneWidth[det] - 1], 0);
                            }

                            // Compute next offset
                            offset += channelSettings.zoneWidth[det];
                        }
                    }
                    else
                    {
                        std::fill(&imageBuffer.get()[lines * channelSettings.totalWidth + 0], &imageBuffer.get()[lines * channelSettings.totalWidth + channelSettings.totalWidth - 1], 0);
                    }

                    // Next line!
                    lines++;
                }

                //double scanTime = 2.3;
                //double oneLine = scanTime / double(channelSettings.zoneHeight);
                //double currentTime = ccsds::parseCCSDSTimeFull(seg.header, -4383);
                //int half = channelSettings.zoneHeight;
                //for (int i = channelSettings.zoneHeight - 1; i >= 0; i--)
                //    timestamps.push_back(currentTime + i * oneLine);
            }

            // Scale bit depth
            for (int i = 0; i < channelSettings.totalWidth * lines; i++)
                imageBuffer.get()[i] *= channelSettings.scale;

            image::Image<uint16_t> image = image::Image<uint16_t>(imageBuffer.get(), channelSettings.totalWidth, lines, 1);

            if (channelSettings.invert)
                image.linear_invert();

            return image;
        }

        // Decompress using USES algorithm
        int uses_decompress(char *input, char *output, int inputLen, int outputLen)
        {
            struct aec_stream strm;
            strm.bits_per_sample = 15;
            strm.block_size = 8;
            strm.rsi = 128;
            strm.flags = AEC_DATA_MSB | AEC_DATA_PREPROCESS;
            strm.next_in = (const unsigned char *)input;
            strm.avail_in = inputLen;
            strm.next_out = (unsigned char *)output;
            strm.avail_out = outputLen * sizeof(char);
            aec_decode_init(&strm);
            aec_decode(&strm, AEC_FLUSH);
            if (aec_decode_init(&strm) != AEC_OK)
                return 1;
            if (aec_decode(&strm, AEC_FLUSH) != AEC_OK)
                return 1;
            aec_decode_end(&strm);
            return 0;
        }
    } // namespace viirs
} // namespace jpss