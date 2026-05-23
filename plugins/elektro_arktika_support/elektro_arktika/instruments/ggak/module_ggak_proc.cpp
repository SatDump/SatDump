#include "module_ggak.h"
#include "ggak_product.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "products/dataset.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace elektro_arktika
{
    namespace ggak
    {
        namespace
        {
            void decode_magnetometer_frame(const uint8_t *payload, size_t len,
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

            void decode_particle_frame(const uint8_t *payload, size_t len,
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

            void decode_subpacket_69(const uint8_t *payload, size_t len,
                                     uint8_t marker_hi,
                                     uint32_t ct, uint8_t ft, uint8_t data_fill,
                                     bool csum_ok, std::vector<SubPacket> &out)
            {
                constexpr int PKT_SIZE = 69;
                const size_t offsets[] = {0, 69, 138};

                for (int p = 0; p < 3; p++)
                {
                    if (offsets[p] + PKT_SIZE > len)
                        break;
                    const uint8_t *r = payload + offsets[p];

                    if (r[0] != marker_hi)
                        continue;
                    if (is_fill_data(r, PKT_SIZE, data_fill))
                        continue;

                    SubPacket pkt{};
                    pkt.seq = r[2];
                    pkt.coarse_time = ct;
                    pkt.fine_time = ft;
                    pkt.checksum_ok = csum_ok;

                    const uint8_t *data = r + 3;
                    int data_len = 65;
                    for (int ch = 0; ch + 1 < data_len; ch += 2)
                        pkt.channels.push_back(static_cast<int32_t>(read_be16(data + ch)));

                    out.push_back(std::move(pkt));
                }
            }

            void decode_housekeeping_12(const uint8_t *payload, size_t len,
                                        uint8_t marker_byte,
                                        uint32_t ct, uint8_t ft, uint8_t data_fill,
                                        bool csum_ok, std::vector<SubPacket> &out)
            {
                constexpr int BLK_SIZE = 12;
                size_t off = 0;

                while (off + BLK_SIZE <= len)
                {
                    const uint8_t *r = payload + off;

                    if (r[0] != marker_byte)
                    {
                        off++;
                        continue;
                    }
                    if (is_fill_data(r, BLK_SIZE, data_fill))
                    {
                        off += BLK_SIZE;
                        continue;
                    }

                    SubPacket pkt{};
                    pkt.seq = r[1];
                    pkt.coarse_time = ct;
                    pkt.fine_time = ft;
                    pkt.checksum_ok = csum_ok;

                    for (int ch = 0; ch < 5; ch++)
                        pkt.channels.push_back(static_cast<int32_t>(read_be16(r + 2 + ch * 2)));

                    out.push_back(std::move(pkt));
                    off += BLK_SIZE;
                }
            }

            void decode_spectral(const uint8_t *payload, size_t len,
                                 uint8_t marker,
                                 uint32_t ct, uint8_t ft, uint8_t data_fill,
                                 bool csum_ok, std::vector<SubPacket> &out)
            {
                size_t off = 0;

                while (off < len && off + SPECTRAL_MIN_PACKET_BYTES <= len)
                {
                    if (payload[off] != marker)
                    {
                        off++;
                        continue;
                    }

                    // Find next marker (minimum distance = SPECTRAL_MIN_PACKET_BYTES)
                    size_t next = off + SPECTRAL_MIN_PACKET_BYTES;
                    while (next < len && payload[next] != marker)
                        next++;
                    size_t end = (next < len) ? next : len;

                    // Strip trailing fill bytes
                    size_t pkt_len = end - off;
                    while (pkt_len > 3 && payload[off + pkt_len - 1] == data_fill)
                        pkt_len--;

                    size_t data_len = pkt_len - 3; // skip marker + type + seq
                    if (pkt_len >= static_cast<size_t>(SPECTRAL_MIN_PACKET_BYTES) && (data_len % 2) == 0)
                    {
                        SubPacket pkt{};
                        pkt.seq = payload[off + 2];
                        pkt.coarse_time = ct;
                        pkt.fine_time = ft;
                        pkt.checksum_ok = csum_ok;

                        const uint8_t *data = payload + off + 3;
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

            CounterGapInfo analyze_counter_gaps(const std::vector<uint16_t> &counters)
            {
                CounterGapInfo info{static_cast<int>(counters.size()), 0, 0};
                for (size_t i = 1; i < counters.size(); i++)
                {
                    int diff = (counters[i] - counters[i - 1] + 65536) % 65536;
                    if (diff > 1 && diff < 60000)
                    {
                        info.gaps++;
                        info.missing_estimate += diff - 1;
                    }
                }
                return info;
            }

            /**
             * Fill small gaps (up to max_gap_frames missed frames) via linear
             * interpolation.  Only suitable for slowly-varying quantities
             * like magnetic field at GEO.
             */
            int interpolate_small_gaps(GGAKChannel &ch, double cadence, int max_gap_frames)
            {
                if (ch.timestamps.size() < 2 || cadence <= 0)
                    return 0;

                double max_gap_dt = (max_gap_frames + 1) * cadence * 0.9;
                int filled = 0;

                std::vector<double> new_ts, new_vals;
                new_ts.reserve(ch.timestamps.size() * 2);
                new_vals.reserve(new_ts.capacity());

                new_ts.push_back(ch.timestamps[0]);
                new_vals.push_back(ch.values[0]);

                for (size_t i = 1; i < ch.timestamps.size(); i++)
                {
                    double dt = ch.timestamps[i] - ch.timestamps[i - 1];
                    if (dt > cadence * 1.5 && dt <= max_gap_dt)
                    {
                        int n_fill = static_cast<int>(std::round(dt / cadence)) - 1;
                        for (int k = 1; k <= n_fill; k++)
                        {
                            double frac = static_cast<double>(k) / (n_fill + 1);
                            double t = ch.timestamps[i - 1] + frac * dt;
                            double v = ch.values[i - 1] + frac * (ch.values[i] - ch.values[i - 1]);
                            new_ts.push_back(t);
                            new_vals.push_back(v);
                            filled++;
                        }
                    }
                    new_ts.push_back(ch.timestamps[i]);
                    new_vals.push_back(ch.values[i]);
                }

                ch.timestamps = std::move(new_ts);
                ch.values = std::move(new_vals);
                return filled;
            }

            void write_magnetometer_csv(const std::vector<MagReading> &readings,
                                        const std::string &path)
            {
                std::ofstream f(path);
                if (!f.is_open())
                    return;

                f << "timestamp,elapsed_s,"
                     "mag_total_raw,mag_x_raw,mag_y_raw,mag_z_raw,"
                     "B_total_nT,Bx_nT,By_nT,Bz_nT,"
                     "voltage_raw,voltage_V,met_counter,"
                     "status,mode,mode2,activity_level,activity_label,"
                     "checksum_ok\n";

                char ts[32];
                for (auto &r : readings)
                {
                    if (!r.is_valid())
                        continue;
                    double es = r.elapsed_seconds();
                    format_timestamp(es, ts, sizeof(ts));

                    f << ts << "," << std::fixed;
                    f.precision(1);
                    f << es << ",";
                    f << r.mag_total_raw << "," << r.mag_x_raw << ","
                      << r.mag_y_raw << "," << r.mag_z_raw << ",";
                    f.precision(2);
                    f << r.mag_total_nt() << "," << r.mag_x_nt() << ","
                      << r.mag_y_nt() << "," << r.mag_z_nt() << ",";
                    f << r.voltage_raw << ",";
                    f.precision(2);
                    f << r.voltage() << ",";
                    f << r.met_counter << ",";
                    f << (int)r.status_flag << "," << (int)r.mode << "," << (int)r.mode2 << ",";
                    f << r.activity_level() << "," << r.activity_label() << ","
                      << (r.checksum_ok ? 1 : 0) << "\n";
                }
            }

            void write_particle_csv(const std::vector<ParticleReading> &readings,
                                    const std::string &path)
            {
                std::ofstream f(path);
                if (!f.is_open())
                    return;

                f << "timestamp,elapsed_s,sub_type,counter";
                for (int i = 0; i < PARTICLE_CHANNELS; i++)
                    f << ",ch_" << (i + 1);
                f << ",checksum_ok\n";

                char ts[32];
                for (auto &r : readings)
                {
                    double es = r.elapsed_seconds();
                    format_timestamp(es, ts, sizeof(ts));

                    f << ts << "," << std::fixed;
                    f.precision(1);
                    f << es << ",0x";

                    // sub_type as hex
                    char hex[8];
                    std::snprintf(hex, sizeof(hex), "%02X", r.sub_type);
                    f << hex << "," << (int)r.counter;

                    for (int i = 0; i < PARTICLE_CHANNELS; i++)
                        f << "," << r.channels[i];
                    f << "," << (r.checksum_ok ? 1 : 0) << "\n";
                }
            }

            void write_subpacket_csv(const std::vector<SubPacket> &packets,
                                     const std::string &path)
            {
                if (packets.empty())
                    return;

                std::ofstream f(path);
                if (!f.is_open())
                    return;

                int max_ch = 0;
                for (auto &p : packets)
                    if (static_cast<int>(p.channels.size()) > max_ch)
                        max_ch = static_cast<int>(p.channels.size());

                f << "timestamp,elapsed_s,seq";
                for (int i = 0; i < max_ch; i++)
                    f << ",ch_" << (i + 1);
                f << ",checksum_ok\n";

                char ts[32];
                for (auto &p : packets)
                {
                    double es = p.elapsed_seconds();
                    format_timestamp(es, ts, sizeof(ts));

                    f << ts << "," << std::fixed;
                    f.precision(1);
                    f << es << "," << (int)p.seq;

                    for (size_t i = 0; i < static_cast<size_t>(max_ch); i++)
                    {
                        f << ",";
                        if (i < p.channels.size())
                            f << p.channels[i];
                        else
                            f << "0";
                    }
                    f << "," << (p.checksum_ok ? 1 : 0) << "\n";
                }
            }
        } // anonymous namespace

        void GGAKDecoderModule::process()
        {
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/GGAK";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directories(directory);

            logger->info("Processing GGAK space environment data...");

            uint8_t cadu[CADU_SIZE];

            int prev_coarse_time = -1;
            uint32_t coarse_time_offset = 0;

            while (should_run())
            {
                read_data(cadu, CADU_SIZE);
                total_frames++;

                // Checksum verification
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
                    ui_total_frames.store(total_frames, std::memory_order_relaxed);
                    ui_fill_frames.store(fill_frames, std::memory_order_relaxed);
                    continue;
                }

                // Auto-detect BND generation from first data frame
                if (!profile_detected)
                {
                    if (hdr.source_id == 0x30)
                        sat_profile = PROFILE_BND_E;
                    else
                        sat_profile = PROFILE_BND_VE;
                    profile_detected = true;
                    logger->info("Detected: %s (source_id=0x%02X)", sat_profile.name, sat_profile.source_id);
                }

                // Track 16-bit coarse_time wraparound (~193 day period)
                if (prev_coarse_time >= 0 && (prev_coarse_time - static_cast<int>(hdr.coarse_time)) > 50000)
                    coarse_time_offset += 65536;
                prev_coarse_time = hdr.coarse_time;

                master_counters.push_back(hdr.master_counter);
                channel_counters[hdr.frame_type].push_back(hdr.channel_counter);

                uint32_t ct = hdr.coarse_time + coarse_time_offset;
                uint8_t ft = hdr.fine_time;
                uint8_t data_fill = sat_profile.data_fill_byte;

                // Payload region: bytes 13..221 of the CADU (after ASM + header)
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
                    decode_spectral(payload, PAYLOAD_SIZE, SPECTRAL_CAL_MARKER, ct, ft, data_fill, csum_ok, subpackets_5c);
                    break;
                case 0x5C:
                    decode_spectral(payload, PAYLOAD_SIZE, SPECTRAL_MARKER, ct, ft, data_fill, csum_ok, subpackets_5c);
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

                ui_total_frames.store(total_frames, std::memory_order_relaxed);
                ui_fill_frames.store(fill_frames, std::memory_order_relaxed);
                ui_mag_count.store((int)mag_readings.size(), std::memory_order_relaxed);
                ui_particle_count.store((int)particle_readings.size(), std::memory_order_relaxed);
                ui_subpackets_20_count.store((int)subpackets_20.size(), std::memory_order_relaxed);
                ui_subpackets_30_count.store((int)subpackets_30.size(), std::memory_order_relaxed);
                ui_subpackets_5c_count.store((int)subpackets_5c.size(), std::memory_order_relaxed);
                ui_subpackets_00_count.store((int)subpackets_00.size(), std::memory_order_relaxed);
                ui_subpackets_10_count.store((int)subpackets_10.size(), std::memory_order_relaxed);
            }

            cleanup();

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

            logger->info("----------- GGAK %s", sat_profile.name);
            logger->info("Total frames        : %d (%d data, %d fill)", total_frames,
                         total_frames - fill_frames, fill_frames);
            logger->info("Checksum            : %d pass / %d fail", checksum_pass, checksum_fail);
            logger->info("FM-VE readings      : %zu", mag_readings.size());
            logger->info("GALS-VE+MIP readings: %zu", particle_readings.size());
            logger->info("SKIF-VE/V ESA pkts  : %zu", subpackets_20.size());
            logger->info("SKIF-VE/G ESA pkts  : %zu", subpackets_30.size());
            logger->info("SKL-E spectral pkts : %zu", subpackets_5c.size());
            logger->info("Housekeeping-A pkts : %zu", subpackets_00.size());
            logger->info("SKIF-VE SER pkts    : %zu", subpackets_10.size());

            CounterGapInfo gap_info = analyze_counter_gaps(master_counters);
            if (gap_info.gaps > 0)
                logger->warn("Counter gaps: %d (est. %d frames missing)", gap_info.gaps, gap_info.missing_estimate);

            std::map<std::string, int> per_inst_gaps, per_inst_missing;
            for (auto &[ft_key, counters] : channel_counters)
            {
                CounterGapInfo ci = analyze_counter_gaps(counters);
                if (ci.gaps > 0)
                {
                    std::string name = frame_type_name(ft_key);
                    per_inst_gaps[name] = ci.gaps;
                    per_inst_missing[name] = ci.missing_estimate;
                    logger->warn("  %s channel gaps: %d (est. %d missing)", name.c_str(), ci.gaps, ci.missing_estimate);
                }
            }

            for (int i = 0; i < STATUS_COUNT; i++)
                inst_status[i] = SAVING;
            inst_status[STATUS_BND] = DONE;

            logger->info("Writing GGAK CSV and metadata...");

            write_magnetometer_csv(mag_readings, directory + "/magnetometer.csv");
            inst_status[STATUS_MAG] = DONE;

            write_particle_csv(particle_readings, directory + "/particles.csv");
            inst_status[STATUS_PARTICLES] = DONE;

            write_subpacket_csv(subpackets_20, directory + "/skif_v_esa.csv");
            write_subpacket_csv(subpackets_30, directory + "/skif_g_esa.csv");
            inst_status[STATUS_SKIF_ESA] = DONE;

            write_subpacket_csv(subpackets_5c, directory + "/skl_spectral.csv");
            inst_status[STATUS_SKL] = DONE;

            write_subpacket_csv(subpackets_00, directory + "/housekeeping.csv");
            write_subpacket_csv(subpackets_10, directory + "/skif_ser.csv");
            inst_status[STATUS_HK] = DONE;

            GGAKProduct ggak_product;
            ggak_product.instrument_name = "ggak";

            {
                constexpr int MAG_CH_COUNT = 5;
                const char *mag_names[MAG_CH_COUNT] = {"|B|", "Bx", "By", "Bz", "Voltage"};
                const char *mag_units[MAG_CH_COUNT] = {"nT", "nT", "nT", "nT", "V"};

                ggak_product.mag_channels.resize(MAG_CH_COUNT);
                for (int i = 0; i < MAG_CH_COUNT; i++)
                {
                    ggak_product.mag_channels[i].name = mag_names[i];
                    ggak_product.mag_channels[i].unit = mag_units[i];
                }

                for (auto &r : mag_readings)
                {
                    if (!r.is_valid())
                        continue;
                    double t = r.elapsed_seconds();
                    for (int i = 0; i < MAG_CH_COUNT; i++)
                        ggak_product.mag_channels[i].timestamps.push_back(t);

                    ggak_product.mag_channels[0].values.push_back(r.mag_total_nt());
                    ggak_product.mag_channels[1].values.push_back(r.mag_x_nt());
                    ggak_product.mag_channels[2].values.push_back(r.mag_y_nt());
                    ggak_product.mag_channels[3].values.push_back(r.mag_z_nt());
                    ggak_product.mag_channels[4].values.push_back(r.voltage());
                }
            }

            if (d_parameters.value("interpolate_mag_gaps", false))
            {
                double mag_cadence = frame_cadence_seconds(0x70);
                int total_filled = 0;
                for (auto &ch : ggak_product.mag_channels)
                    total_filled += interpolate_small_gaps(ch, mag_cadence, 2);
                if (total_filled > 0)
                    logger->info("Interpolated %d magnetometer gap samples", total_filled);
            }

            for (auto &ch : ggak_product.mag_channels)
                ch.updateMinMax();

            {
                const char *particle_names[PARTICLE_CHANNELS] = {
                    "Ep>=600MeV", "Ep>=800MeV", "Ep>=1100MeV",
                    "Cg-1", "Cg-2", "Cg-3", "Cg-4", "MIP"};

                ggak_product.particle_channels.resize(PARTICLE_CHANNELS);
                for (int ch = 0; ch < PARTICLE_CHANNELS; ch++)
                {
                    ggak_product.particle_channels[ch].name = particle_names[ch];
                    ggak_product.particle_channels[ch].unit = "Counts";
                }

                for (auto &r : particle_readings)
                {
                    double t = r.elapsed_seconds();
                    for (int ch = 0; ch < PARTICLE_CHANNELS; ch++)
                    {
                        ggak_product.particle_channels[ch].timestamps.push_back(t);
                        ggak_product.particle_channels[ch].values.push_back(static_cast<double>(r.channels[ch]));
                    }
                }

                for (auto &ch : ggak_product.particle_channels)
                    ch.updateMinMax();
            }

            auto make_subpkt_avg = [](const char *name, const char *unit, const char *group,
                                      const std::vector<SubPacket> &data) -> GGAKChannel {
                GGAKChannel gc;
                gc.name = name;
                gc.unit = unit;
                gc.group = group;
                for (auto &p : data)
                {
                    double avg = 0;
                    if (!p.channels.empty())
                    {
                        for (auto v : p.channels)
                            avg += v;
                        avg /= p.channels.size();
                    }
                    gc.timestamps.push_back(p.elapsed_seconds());
                    gc.values.push_back(avg);
                }
                gc.updateMinMax();
                return gc;
            };

            auto make_subpkt_ch = [](const char *name, const char *unit, const char *group,
                                     const std::vector<SubPacket> &data, int ch_idx,
                                     double scale = 1.0) -> GGAKChannel {
                GGAKChannel gc;
                gc.name = name;
                gc.unit = unit;
                gc.group = group;
                for (auto &p : data)
                {
                    if (ch_idx < (int)p.channels.size())
                    {
                        gc.timestamps.push_back(p.elapsed_seconds());
                        gc.values.push_back(p.channels[ch_idx] * scale);
                    }
                }
                gc.updateMinMax();
                return gc;
            };

            ggak_product.subpacket_channels.push_back(
                make_subpkt_avg("SKIF-VE/V ESA (avg)", "counts", "SKIF-VE/V ESA", subpackets_20));
            ggak_product.subpacket_channels.push_back(
                make_subpkt_avg("SKIF-VE/G ESA (avg)", "counts", "SKIF-VE/G ESA", subpackets_30));
            ggak_product.subpacket_channels.push_back(
                make_subpkt_avg("SKL-E Spectral (avg)", "counts (signed)", "SKL-E Spectral", subpackets_5c));

            ggak_product.subpacket_channels.push_back(
                make_subpkt_ch("ISP-2M TSI", "W/m\xC2\xB2", "Housekeeping-A", subpackets_00, 2, 0.0331));
            ggak_product.subpacket_channels.push_back(
                make_subpkt_ch("BND-VE Thermal", "counts (14-bit)", "Housekeeping-A", subpackets_00, 1));
            ggak_product.subpacket_channels.push_back(
                make_subpkt_ch("HK Mux (ch_1)", "counts", "Housekeeping-A", subpackets_00, 0));

            ggak_product.subpacket_channels.push_back(
                make_subpkt_ch("SKIF-VE/V SER", "cts/frame", "SKIF-VE SER summary", subpackets_10, 2));
            ggak_product.subpacket_channels.push_back(
                make_subpkt_ch("SKIF-VE/G SER", "cts/frame", "SKIF-VE SER summary", subpackets_10, 4));

            int valid_mag = ggak_product.mag_channels.empty()
                                ? 0
                                : (int)ggak_product.mag_channels[0].values.size();

            nlohmann::json meta;
            meta["instrument"] = "GGAK";
            meta["generation"] = sat_profile.generation;
            meta["satellite_name"] = sat_profile.name;
            meta["source_id"] = sat_profile.source_id;
            meta["total_frames"] = total_frames;
            meta["data_frames"] = total_frames - fill_frames;
            meta["fill_frames"] = fill_frames;
            meta["checksum"] = {{"pass", checksum_pass}, {"fail", checksum_fail}};

            nlohmann::json fc;
            for (auto &[ft_key, count] : frame_counts)
            {
                char key[8];
                std::snprintf(key, sizeof(key), "0x%02X", ft_key);
                fc[key] = count;
            }
            meta["frame_counts"] = fc;

            meta["counter_analysis"] = {
                {"total_frames", gap_info.total_frames},
                {"gaps", gap_info.gaps},
                {"missing_estimate", gap_info.missing_estimate}};

            meta["timing"] = {
                {"tick_rate_seconds", TIME_TICK_SECONDS},
                {"fine_step_seconds", FINE_TIME_STEP},
                {"capture_frames", total_frames}};

            meta["magnetometer"] = {
                {"total_readings", (int)mag_readings.size()},
                {"valid_readings", valid_mag},
                {"calibration", {{"adc_offset", MAG_ADC_OFFSET}, {"scale_nT_per_count", MAG_SCALE}, {"formula", "nT = (raw - 2339) * 2.359"}}}};

            meta["particles"] = {
                {"total_readings", (int)particle_readings.size()},
                {"channels", PARTICLE_CHANNELS},
                {"channel_mapping", {{"ch_1", "GALS-VE/-CH Ep >= 600 MeV"}, {"ch_2", "GALS-VE/-CH Ep >= 800 MeV"}, {"ch_3", "GALS-VE/-CH Ep >= 1100 MeV"}, {"ch_4", "GALS-VE/-S Cg-1 Ee > 0.15 MeV, Ep > 5 MeV"}, {"ch_5", "GALS-VE/-S Cg-2 Ee > 0.7 MeV, Ep > 15 MeV"}, {"ch_6", "GALS-VE/-S Cg-3 Ee > 1.7 MeV, Ep > 25 MeV"}, {"ch_7", "GALS-VE/-S Cg-4 Ee > 3.2 MeV, Ep > 40 MeV"}, {"ch_8", "SKIF-VE MIP Ep > 15 MeV, Ee > 800 keV"}}}};

            meta["subpackets"] = {
                {"skif_ve_v_esa", (int)subpackets_20.size()},
                {"skif_ve_g_esa", (int)subpackets_30.size()},
                {"skl_e_spectral", (int)subpackets_5c.size()},
                {"housekeeping_a", (int)subpackets_00.size()},
                {"skif_ve_ser", (int)subpackets_10.size()}};

            saveJsonFile(directory + "/metadata.json", meta);

            ggak_product.stats.total_frames = total_frames;
            ggak_product.stats.data_frames = total_frames - fill_frames;
            ggak_product.stats.fill_frames = fill_frames;
            ggak_product.stats.checksum_pass = checksum_pass;
            ggak_product.stats.checksum_fail = checksum_fail;
            ggak_product.stats.counter_gaps = gap_info.gaps;
            ggak_product.stats.missing_estimate = gap_info.missing_estimate;
            ggak_product.stats.generation = sat_profile.generation;
            ggak_product.stats.satellite_name = sat_profile.name;
            ggak_product.stats.per_instrument_gaps = per_inst_gaps;
            ggak_product.stats.per_instrument_missing = per_inst_missing;

            ggak_product.set_product_source(sat_profile.name);
            ggak_product.set_product_timestamp(time(0));
            ggak_product.save(directory);

            std::string sat_name = "ELEKTRO-L";
            if (sat_profile.source_id == 0x30)
                sat_name = "ELEKTRO-L (GGAK-E)";
            else
                sat_name = "ELEKTRO-L (GGAK-VE)";

            satdump::products::DataSet dataset;
            dataset.satellite_name = sat_name;
            dataset.timestamp = time(0);
            dataset.products_list.push_back("GGAK");
            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));

            logger->info("GGAK processing complete.");
        }
    } // namespace ggak
} // namespace elektro_arktika
