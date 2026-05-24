#include "ggak_reader.h"
#include "logger.h"

#include <algorithm>

namespace elektro_arktika
{
    namespace ggak
    {
        void GGAKReader::pushFrame(const uint8_t *cadu)
        {
            total_frames++;

            bool csum_ok = validate_checksum(cadu);
            if (csum_ok)
                checksum_pass++;
            else
                checksum_fail++;

            GGAKFrameHeader hdr = parse_header(cadu);
            frame_counts[hdr.frame_type]++;

            if (is_fill_frame(hdr.frame_type))
            {
                fill_frames++;
                return;
            }

            if (!profile_detected)
            {
                observed_source_id = hdr.source_id;
                if (hdr.source_id == 0x30)
                    sat_profile = PROFILE_BND_E;
                else if (hdr.source_id == 0x31 || hdr.source_id == 0x33)
                    sat_profile = PROFILE_BND_VE;
                else
                {
                    logger->warn("Unknown GGAK source_id 0x%02X, assuming BND-VE", hdr.source_id);
                    sat_profile = PROFILE_BND_VE;
                }
                profile_detected = true;
                logger->info("Detected: %s (source_id=0x%02X)", sat_profile.name, hdr.source_id);
            }

            if (prev_coarse_time >= 0 && (prev_coarse_time - static_cast<int>(hdr.coarse_time)) > 50000)
                coarse_time_offset += 65536;
            prev_coarse_time = hdr.coarse_time;

            master_counters.push_back(hdr.master_counter);
            channel_counters[hdr.frame_type].push_back(hdr.channel_counter);

            uint32_t ct = hdr.coarse_time + coarse_time_offset;
            uint8_t ft = hdr.fine_time;
            uint8_t data_fill = sat_profile.data_fill_byte;
            const uint8_t *payload = cadu + PAYLOAD_OFFSET;

            switch (hdr.frame_type)
            {
            case 0x70:
                decode_magnetometer_frame(payload, PAYLOAD_SIZE, ct, ft, data_fill, csum_ok, mag_readings);
                break;
            case 0x40:
                decode_particle_frame(payload, PAYLOAD_SIZE, ct, ft, data_fill, csum_ok, particle_readings);
                break;
            case 0x20:
                decode_subpacket_69(payload, PAYLOAD_SIZE, 0x90, ct, ft, data_fill, csum_ok, subpackets_20);
                break;
            case 0x30:
                decode_subpacket_69(payload, PAYLOAD_SIZE, 0x98, ct, ft, data_fill, csum_ok, subpackets_30);
                break;
            case 0x50:
                decode_spectral(payload, PAYLOAD_SIZE, SPECTRAL_CAL_MARKER, ct, ft, data_fill, csum_ok, subpackets_5c, spectral_buffer);
                break;
            case 0x5C:
                decode_spectral(payload, PAYLOAD_SIZE, SPECTRAL_MARKER, ct, ft, data_fill, csum_ok, subpackets_5c, spectral_buffer);
                break;
            case 0x00:
                decode_housekeeping_12(payload, PAYLOAD_SIZE, 0x80, ct, ft, data_fill, csum_ok, subpackets_00);
                break;
            case 0x10:
                decode_housekeeping_12(payload, PAYLOAD_SIZE, 0x88, ct, ft, data_fill, csum_ok, subpackets_10);
                break;
            default:
                break;
            }
        }

        void GGAKReader::sortAll()
        {
            auto by_time = [](const auto &a, const auto &b) {
                return a.elapsed_seconds() < b.elapsed_seconds();
            };
            std::sort(mag_readings.begin(), mag_readings.end(), by_time);
            std::sort(particle_readings.begin(), particle_readings.end(), by_time);
            std::sort(subpackets_20.begin(), subpackets_20.end(), by_time);
            std::sort(subpackets_30.begin(), subpackets_30.end(), by_time);
            std::sort(subpackets_5c.begin(), subpackets_5c.end(), by_time);
            std::sort(subpackets_00.begin(), subpackets_00.end(), by_time);
            std::sort(subpackets_10.begin(), subpackets_10.end(), by_time);
        }

        void GGAKReader::decode_magnetometer_frame(const uint8_t *payload, size_t len,
                                                   uint32_t ct, uint8_t ft, uint8_t data_fill,
                                                   bool csum_ok, std::vector<MagReading> &out)
        {
            for (int i = 0; i < MAG_READINGS_PER_FRAME; i++)
            {
                size_t off = i * MAG_READING_LEN;
                if (off + MAG_READING_LEN > len)
                    break;
                const uint8_t *r = payload + off;

                if (is_fill_data(r, MAG_READING_LEN, data_fill))
                    continue;

                MagReading rd{};
                rd.mag_total_raw = read_be16(r + 0);
                rd.mag_x_raw = read_be16(r + 2);
                rd.mag_y_raw = read_be16(r + 4);
                rd.mag_z_raw = read_be16(r + 6);
                rd.voltage_raw = read_be16(r + 8);
                rd.met_counter = read_be16(r + 10);
                rd.status_flag = r[12];
                rd.mode = r[13];
                rd.mode2 = r[14];
                rd.coarse_time = ct;
                rd.fine_time = ft;
                rd.checksum_ok = csum_ok;
                out.push_back(rd);
            }
        }

        void GGAKReader::decode_particle_frame(const uint8_t *payload, size_t len,
                                               uint32_t ct, uint8_t ft, uint8_t data_fill,
                                               bool csum_ok, std::vector<ParticleReading> &out)
        {
            for (int i = 0; i < PARTICLE_READINGS_PER_FRAME; i++)
            {
                size_t off = i * PARTICLE_READING_LEN;
                if (off + PARTICLE_READING_LEN > len)
                    break;
                const uint8_t *r = payload + off;

                if (r[0] != PARTICLE_MARKER_A0 && r[0] != PARTICLE_MARKER_A1)
                    continue;
                if (is_fill_data(r, PARTICLE_READING_LEN, data_fill))
                    continue;

                ParticleReading rd{};
                rd.sub_type = r[1];
                rd.counter = r[2];
                for (int ch = 0; ch < PARTICLE_CHANNELS; ch++)
                    rd.channels[ch] = read_be16(r + 3 + ch * 2);
                rd.coarse_time = ct;
                rd.fine_time = ft;
                rd.checksum_ok = csum_ok;
                out.push_back(rd);
            }
        }

        void GGAKReader::decode_subpacket_69(const uint8_t *payload, size_t len,
                                             uint8_t marker_hi,
                                             uint32_t ct, uint8_t ft, uint8_t data_fill,
                                             bool csum_ok, std::vector<SubPacket> &out)
        {
            for (int p = 0; p < 3; p++)
            {
                if (SKIF_ESA_OFFSETS[p] + SKIF_ESA_PKT_SIZE > len)
                    break;
                const uint8_t *r = payload + SKIF_ESA_OFFSETS[p];

                if (r[0] != marker_hi)
                    continue;
                if (is_fill_data(r, SKIF_ESA_PKT_SIZE, data_fill))
                    continue;

                SubPacket pkt{};
                pkt.seq = r[2];
                pkt.coarse_time = ct;
                pkt.fine_time = ft;
                pkt.checksum_ok = csum_ok;

                const uint8_t *data = r + 3;
                for (int ch = 0; ch + 1 < SKIF_ESA_DATA_LEN; ch += 2)
                    pkt.channels.push_back(static_cast<int32_t>(read_be16(data + ch)));

                out.push_back(std::move(pkt));
            }
        }

        void GGAKReader::decode_housekeeping_12(const uint8_t *payload, size_t len,
                                                uint8_t marker_byte,
                                                uint32_t ct, uint8_t ft, uint8_t data_fill,
                                                bool csum_ok, std::vector<SubPacket> &out)
        {
            size_t off = 0;

            while (off + HK_BLK_SIZE <= len)
            {
                const uint8_t *r = payload + off;

                if (r[0] != marker_byte)
                {
                    off++;
                    continue;
                }
                if (is_fill_data(r, HK_BLK_SIZE, data_fill))
                {
                    off += HK_BLK_SIZE;
                    continue;
                }

                SubPacket pkt{};
                pkt.seq = r[1];
                pkt.coarse_time = ct;
                pkt.fine_time = ft;
                pkt.checksum_ok = csum_ok;

                for (int ch = 0; ch < HK_CHANNELS; ch++)
                    pkt.channels.push_back(static_cast<int32_t>(read_be16(r + 2 + ch * 2)));

                out.push_back(std::move(pkt));
                off += HK_BLK_SIZE;
            }
        }

        void GGAKReader::decode_spectral(const uint8_t *payload, size_t len,
                                         uint8_t marker,
                                         uint32_t ct, uint8_t ft, uint8_t data_fill,
                                         bool csum_ok, std::vector<SubPacket> &out,
                                         std::vector<uint8_t> &buffer)
        {
            std::vector<uint8_t> data_to_process;
            data_to_process.reserve(buffer.size() + len);
            data_to_process.insert(data_to_process.end(), buffer.begin(), buffer.end());
            data_to_process.insert(data_to_process.end(), payload, payload + len);
            buffer.clear();

            size_t off = 0;
            size_t total_len = data_to_process.size();

            while (off < total_len)
            {
                if (data_to_process[off] != marker)
                {
                    off++;
                    continue;
                }

                if (off + SPECTRAL_MIN_PACKET_BYTES > total_len)
                {
                    buffer.insert(buffer.end(), data_to_process.begin() + off, data_to_process.end());
                    break;
                }

                size_t next = off + SPECTRAL_MIN_PACKET_BYTES;
                while (next < total_len && data_to_process[next] != marker)
                    next++;

                size_t end = next;

                if (next == total_len)
                {
                    size_t pkt_len = end - off;
                    size_t check_len = pkt_len;
                    while (check_len > 3 && data_to_process[off + check_len - 1] == data_fill)
                        check_len--;

                    if (check_len == pkt_len)
                    {
                        buffer.insert(buffer.end(), data_to_process.begin() + off, data_to_process.end());
                        break;
                    }
                }

                size_t pkt_len = end - off;
                while (pkt_len > 3 && data_to_process[off + pkt_len - 1] == data_fill)
                    pkt_len--;

                size_t data_len = pkt_len - 3;
                if (pkt_len >= static_cast<size_t>(SPECTRAL_MIN_PACKET_BYTES) && (data_len % 2) == 0)
                {
                    SubPacket pkt{};
                    pkt.seq = data_to_process[off + 2];
                    pkt.coarse_time = ct;
                    pkt.fine_time = ft;
                    pkt.checksum_ok = csum_ok;

                    const uint8_t *data = data_to_process.data() + off + 3;
                    for (size_t ch = 0; ch + 1 < data_len; ch += 2)
                        pkt.channels.push_back(static_cast<int32_t>(read_be16s(data + ch)));

                    out.push_back(std::move(pkt));
                    off = end;
                }
                else
                {
                    off++;
                }
            }
        }
    } // namespace ggak
} // namespace elektro_arktika
