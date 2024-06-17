#include "channel_reader.h"
#include "common/ccsds/ccsds_time.h"

namespace jpss
{
    namespace viirs
    {
        VIIRSReader::VIIRSReader(Channel &ch)
            : current_segment(ch),
              channelSettings(ch)
        {
            in_segment = false;
            currentSegment = 0;

            aec_cfg.bits_per_sample = 15;
            aec_cfg.block_size = 8;
            aec_cfg.rsi = 128;
            aec_cfg.flags = AEC_DATA_MSB | AEC_DATA_PREPROCESS;
        }

        VIIRSReader::~VIIRSReader()
        {
        }

        void VIIRSReader::feed(ccsds::CCSDSPacket &packet)
        {
            if (packet.header.apid != channelSettings.apid) // Filter out the channel we want
                return;

            if (packet.header.sequence_flag == 1) // Segment header
            {
                in_segment = true;
                segments.push_back(VIIRS_Segment(channelSettings));

                // Parse what we need
                int number_of_packets = packet.payload[8];
                get_current_seg().timestamp = ccsds::parseCCSDSTimeFull(packet, -4383);

                // Compute last segment
                endSequenceCount = packet.header.packet_sequence_count + number_of_packets + 2;
            }
            else if (in_segment && packet.header.packet_sequence_count <= endSequenceCount) // Segment body
            {
                // First, get what we need out of the header
                int detector = packet.payload[19];
                uint32_t sync_word_pattern = packet.payload[20] << 24 | packet.payload[21] << 16 | packet.payload[22] << 8 | packet.payload[23];

                // Find all detectors
                int det_offset = 88;
                for (int det_n = 0; det_n < 6; det_n++)
                {
                    if (det_offset < (int)packet.payload.size())
                    {
                        uint8_t *detector_ptr = &packet.payload[det_offset];

                        // Parse stuff
                        uint8_t fill_size = detector_ptr[0];
                        uint16_t checksum_offset = detector_ptr[2] << 8 | detector_ptr[3];
                        int data_payload_size = checksum_offset - 4;

                        // Check this detector is valid
                        if (data_payload_size <= 0)
                            continue;
                        if (checksum_offset < 4 || checksum_offset >= (packet.payload.size() - det_offset) - 4)
                            continue;

                        // Parse syncword
                        uint32_t sync_word = 0xC000FFEE;
                        if ((packet.payload.size() - det_offset) > 8)
                            sync_word = detector_ptr[checksum_offset + 4] << 24 |
                                        detector_ptr[checksum_offset + 5] << 16 |
                                        detector_ptr[checksum_offset + 6] << 8 |
                                        detector_ptr[checksum_offset + 7];

                        // Slice down payload size to actual size
                        bit_slicer_detector(data_payload_size, fill_size);

                        if (data_payload_size > 8 && (sync_word == sync_word_pattern || sync_word == 0xC000FFEE))
                        {
                            VIIRS_Segment &current_segment = get_current_seg();

                            if (detector > channelSettings.zoneHeight)
                                continue;

                            // Decompress
                            aec_cfg.next_in = (const unsigned char *)&detector_ptr[4];
                            aec_cfg.avail_in = data_payload_size - 1;
                            aec_cfg.next_out = (unsigned char *)current_segment.detector_data[detector][det_n].data();
                            aec_cfg.avail_out = current_segment.detector_data[detector][det_n].size() * 2;
                            aec_decode_init(&aec_cfg);
                            aec_decode(&aec_cfg, AEC_FLUSH);
                            aec_decode_end(&aec_cfg);

                            // We read as big-endian, now convert to little endian for this to work...
                            for (int y = 0; y < (int)current_segment.detector_data[detector][det_n].size(); y++)
                                current_segment.detector_data[detector][det_n][y] = ((current_segment.detector_data[detector][det_n][y] & 0xff) << 8) | ((current_segment.detector_data[detector][det_n][y] & 0xff00) >> 8);

                            // If this detector is oversampled, decimate and average samples
                            if (channelSettings.oversampleZone[det_n] > 1)
                            {
                                double val = 0;
                                for (int y = 0; y < channelSettings.zoneWidth[det_n] * channelSettings.oversampleZone[det_n]; y += channelSettings.oversampleZone[det_n])
                                {
                                    if (channelSettings.oversampleZone[det_n] == 2)
                                        val = (current_segment.detector_data[detector][det_n][y] + current_segment.detector_data[detector][det_n][y + 1]) / channelSettings.oversampleZone[det_n];
                                    else if (channelSettings.oversampleZone[det_n] == 3)
                                        val = (current_segment.detector_data[detector][det_n][y] + current_segment.detector_data[detector][det_n][y + 1] + current_segment.detector_data[detector][det_n][y + 2]) / channelSettings.oversampleZone[det_n];
                                    current_segment.detector_data[detector][det_n][y / channelSettings.oversampleZone[det_n]] = val;
                                }
                            }
                        }

                        det_offset += checksum_offset + 8;
                    }
                }
            }
        }

        void VIIRSReader::differentialDecode(VIIRSReader &channelSource, int decimation)
        {
            for (VIIRS_Segment &seg : segments)
            {
                bool found = false;
                for (VIIRS_Segment &seg_init : channelSource.segments)
                {
                    if (seg.timestamp == seg_init.timestamp)
                    {
                        for (int seg_line = 0; seg_line < channelSettings.zoneHeight; seg_line++)
                            for (int det_n = 0; det_n < 6; det_n++)
                                for (int y = 0; y < channelSettings.zoneWidth[det_n]; y++)
                                    seg.detector_data[seg_line][det_n][y] = (int)(seg.detector_data[seg_line][det_n][y] + seg_init.detector_data[seg_line / decimation][det_n][y / decimation]) - (int)16383;
                        found = true;
                    }
                }

                if (!found) // If we can't decode, set it to black
                    seg = VIIRS_Segment(channelSettings);
            }
        }

        image::Image VIIRSReader::getImage()
        {
            image::Image image(16, channelSettings.totalWidth, channelSettings.zoneHeight * (segments.size() + 1), 1);
            timestamps.clear();

            // Recompose image
            for (size_t seg_n = 0; seg_n < segments.size(); seg_n++)
            {
                VIIRS_Segment &segment = segments[seg_n];

                for (size_t seg_line = 0; seg_line < (size_t)channelSettings.zoneHeight; seg_line++)
                {
                    size_t current_line = seg_n * channelSettings.zoneHeight + ((channelSettings.zoneHeight - 1) - seg_line);
                    size_t det_offset = 0;

                    for (size_t det_n = 0; det_n < 6; det_n++)
                    {
                        // Copy & Scale bit depth
                        for (int i = 0; i < channelSettings.zoneWidth[det_n]; i++)
                            image.set(current_line * channelSettings.totalWidth + det_offset + i, segment.detector_data[seg_line][det_n][i] * channelSettings.scale);

                        // Compute next offset
                        det_offset += channelSettings.zoneWidth[det_n];
                    }
                }

                timestamps.push_back(segment.timestamp);
            }

            return image;
        }
    } // namespace viirs
} // namespace jpss