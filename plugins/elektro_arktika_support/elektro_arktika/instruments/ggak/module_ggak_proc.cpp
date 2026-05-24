#include "module_ggak.h"
#include "ggak_product.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "products/dataset.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace elektro_arktika
{
    namespace ggak
    {
        namespace
        {
            CounterGapInfo analyze_counter_gaps(const std::vector<uint16_t> &counters)
            {
                CounterGapInfo info{static_cast<int>(counters.size()), 0, 0};
                for (size_t i = 1; i < counters.size(); i++)
                {
                    int diff = (counters[i] - counters[i - 1] + 65536) % 65536;
                    if (diff > 1 && diff < COUNTER_GAP_WRAP_THRESHOLD)
                    {
                        info.gaps++;
                        info.missing_estimate += diff - 1;
                    }
                }
                return info;
            }

            /**
             * Fill small gaps (up to max_gap_frames missed frames) via linear
             * interpolation. Only suitable for slowly-varying quantities
             * like magnetic field at GEO orbit where changes between adjacent
             * samples are small relative to the measurement range.
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

            GGAKChannel make_subpkt_avg(const char *name, const char *unit, const char *group,
                                        const std::vector<SubPacket> &data)
            {
                GGAKChannel gc;
                gc.name = name;
                gc.unit = unit;
                gc.group = group;
                for (const auto &p : data)
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
            }

            GGAKChannel make_subpkt_ch(const char *name, const char *unit, const char *group,
                                       const std::vector<SubPacket> &data, int ch_idx,
                                       double scale = 1.0)
            {
                GGAKChannel gc;
                gc.name = name;
                gc.unit = unit;
                gc.group = group;
                for (const auto &p : data)
                {
                    if (ch_idx < static_cast<int>(p.channels.size()))
                    {
                        gc.timestamps.push_back(p.elapsed_seconds());
                        gc.values.push_back(p.channels[ch_idx] * scale);
                    }
                }
                gc.updateMinMax();
                return gc;
            }

            constexpr int SKIF_ESA_BINS = (SKIF_ESA_DATA_LEN - 1) / 2;

            void make_subpkt_bins(const char *group,
                                  const std::vector<SubPacket> &data,
                                  std::vector<GGAKChannel> &out)
            {
                for (int b = 0; b < SKIF_ESA_BINS; b++)
                {
                    GGAKChannel gc;
                    gc.group = group;
                    gc.unit = "counts";
                    char name_buf[32];
                    if (b < SKIF_VE_ESA_STEPS)
                        std::snprintf(name_buf, sizeof(name_buf), "%.2g keV", SKIF_VE_ESA_ENERGIES_KEV[b]);
                    else
                        std::snprintf(name_buf, sizeof(name_buf), "bin %d", b + 1);
                    gc.name = name_buf;

                    for (const auto &p : data)
                    {
                        if (b < static_cast<int>(p.channels.size()))
                        {
                            gc.timestamps.push_back(p.elapsed_seconds());
                            gc.values.push_back(static_cast<double>(p.channels[b]));
                        }
                    }
                    gc.updateMinMax();
                    out.push_back(std::move(gc));
                }
            }
        } // anonymous namespace

        void GGAKDecoderModule::refreshUiCounters()
        {
            ui_total_frames.store(reader.total_frames, std::memory_order_relaxed);
            ui_fill_frames.store(reader.fill_frames, std::memory_order_relaxed);
            ui_mag_count.store(static_cast<int>(reader.mag_readings.size()), std::memory_order_relaxed);
            ui_particle_count.store(static_cast<int>(reader.particle_readings.size()), std::memory_order_relaxed);
            ui_subpackets_20_count.store(static_cast<int>(reader.subpackets_20.size()), std::memory_order_relaxed);
            ui_subpackets_30_count.store(static_cast<int>(reader.subpackets_30.size()), std::memory_order_relaxed);
            ui_subpackets_5c_count.store(static_cast<int>(reader.subpackets_5c.size()), std::memory_order_relaxed);
            ui_subpackets_00_count.store(static_cast<int>(reader.subpackets_00.size()), std::memory_order_relaxed);
            ui_subpackets_10_count.store(static_cast<int>(reader.subpackets_10.size()), std::memory_order_relaxed);
        }

        void GGAKDecoderModule::decodeFrames()
        {
            uint8_t cadu[CADU_SIZE];

            while (should_run())
            {
                read_data(cadu, CADU_SIZE);
                reader.pushFrame(cadu);

                if ((reader.total_frames & 0x3F) == 0)
                    refreshUiCounters();
            }

            refreshUiCounters();

            cleanup();
            reader.sortAll();
        }

        void GGAKDecoderModule::logSummary()
        {
            logger->info("----------- GGAK %s", reader.sat_profile.name);
            logger->info("Total frames        : %d (%d data, %d fill)", reader.total_frames,
                         reader.total_frames - reader.fill_frames, reader.fill_frames);
            logger->info("Checksum            : %d pass / %d fail", reader.checksum_pass, reader.checksum_fail);
            logger->info("FM-VE readings      : %zu", reader.mag_readings.size());
            logger->info("GALS-VE+MIP readings: %zu", reader.particle_readings.size());
            logger->info("SKIF-VE/V ESA pkts  : %zu", reader.subpackets_20.size());
            logger->info("SKIF-VE/G ESA pkts  : %zu", reader.subpackets_30.size());
            logger->info("SKL-E spectral pkts : %zu", reader.subpackets_5c.size());
            logger->info("Housekeeping-A pkts : %zu", reader.subpackets_00.size());
            logger->info("SKIF-VE SER pkts    : %zu", reader.subpackets_10.size());

            master_gap_info = analyze_counter_gaps(reader.master_counters);
            if (master_gap_info.gaps > 0)
                logger->warn("Counter gaps: %d (est. %d frames missing)",
                             master_gap_info.gaps, master_gap_info.missing_estimate);

            for (auto &[ft_key, counters] : reader.channel_counters)
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
        }

        void GGAKDecoderModule::buildProduct(GGAKProduct &product)
        {
            product.mag_channels.clear();
            product.particle_channels.clear();
            product.subpacket_channels.clear();
            product.stats = {};
            product.instrument_name = "ggak";

            {
                constexpr int MAG_CH_COUNT = 5;
                const char *mag_names[MAG_CH_COUNT] = {"|B|", "Bx", "By", "Bz", "Voltage"};
                const char *mag_units[MAG_CH_COUNT] = {"nT", "nT", "nT", "nT", "V"};

                product.mag_channels.resize(MAG_CH_COUNT);
                for (int i = 0; i < MAG_CH_COUNT; i++)
                {
                    product.mag_channels[i].name = mag_names[i];
                    product.mag_channels[i].unit = mag_units[i];
                }

                for (const auto &r : reader.mag_readings)
                {
                    if (!r.is_valid())
                        continue;
                    double t = r.elapsed_seconds();
                    for (int i = 0; i < MAG_CH_COUNT; i++)
                        product.mag_channels[i].timestamps.push_back(t);

                    product.mag_channels[0].values.push_back(r.mag_total_nt());
                    product.mag_channels[1].values.push_back(r.mag_x_nt());
                    product.mag_channels[2].values.push_back(r.mag_y_nt());
                    product.mag_channels[3].values.push_back(r.mag_z_nt());
                    product.mag_channels[4].values.push_back(r.voltage());
                }
            }

            if (d_parameters.value("interpolate_mag_gaps", false))
            {
                double mag_cadence = frame_cadence_seconds(0x70);
                int total_filled = 0;
                for (auto &ch : product.mag_channels)
                    total_filled += interpolate_small_gaps(ch, mag_cadence, 2);
                if (total_filled > 0)
                    logger->info("Interpolated %d magnetometer gap samples", total_filled);
            }

            for (auto &ch : product.mag_channels)
                ch.updateMinMax();

            {
                const char *particle_names[PARTICLE_CHANNELS] = {
                    "Ep>=600MeV", "Ep>=800MeV", "Ep>=1100MeV",
                    "Cg-1", "Cg-2", "Cg-3", "Cg-4", "MIP"};

                product.particle_channels.resize(PARTICLE_CHANNELS);
                for (int ch = 0; ch < PARTICLE_CHANNELS; ch++)
                {
                    product.particle_channels[ch].name = particle_names[ch];
                    product.particle_channels[ch].unit = "Counts";
                }

                for (const auto &r : reader.particle_readings)
                {
                    double t = r.elapsed_seconds();
                    for (int ch = 0; ch < PARTICLE_CHANNELS; ch++)
                    {
                        product.particle_channels[ch].timestamps.push_back(t);
                        product.particle_channels[ch].values.push_back(static_cast<double>(r.channels[ch]));
                    }
                }

                for (auto &ch : product.particle_channels)
                    ch.updateMinMax();
            }

            make_subpkt_bins("SKIF-VE/V ESA", reader.subpackets_20, product.subpacket_channels);
            make_subpkt_bins("SKIF-VE/G ESA", reader.subpackets_30, product.subpacket_channels);
            product.subpacket_channels.push_back(
                make_subpkt_avg("SKL-E Spectral (avg)", "counts (signed)", "SKL-E Spectral", reader.subpackets_5c));

            product.subpacket_channels.push_back(
                make_subpkt_ch("ISP-2M TSI", "W/m\xC2\xB2", "Housekeeping-A", reader.subpackets_00, 2, 0.0331));
            product.subpacket_channels.push_back(
                make_subpkt_ch("BND-VE Thermal", "counts (14-bit)", "Housekeeping-A", reader.subpackets_00, 1));
            product.subpacket_channels.push_back(
                make_subpkt_ch("HK Mux (ch_1)", "counts", "Housekeeping-A", reader.subpackets_00, 0));

            product.subpacket_channels.push_back(
                make_subpkt_ch("SKIF-VE/V SER", "cts/frame", "SKIF-VE SER summary", reader.subpackets_10, 2));
            product.subpacket_channels.push_back(
                make_subpkt_ch("SKIF-VE/G SER", "cts/frame", "SKIF-VE SER summary", reader.subpackets_10, 4));

            product.stats.total_frames = reader.total_frames;
            product.stats.data_frames = reader.total_frames - reader.fill_frames;
            product.stats.fill_frames = reader.fill_frames;
            product.stats.checksum_pass = reader.checksum_pass;
            product.stats.checksum_fail = reader.checksum_fail;
            product.stats.counter_gaps = master_gap_info.gaps;
            product.stats.missing_estimate = master_gap_info.missing_estimate;
            product.stats.generation = reader.sat_profile.generation;
            product.stats.satellite_name = reader.sat_profile.name;
            product.stats.observed_source_id = reader.observed_source_id;
            product.stats.per_instrument_gaps = per_inst_gaps;
            product.stats.per_instrument_missing = per_inst_missing;
        }

        void GGAKDecoderModule::process()
        {
            std::string directory = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/GGAK";

            if (!std::filesystem::exists(directory))
                std::filesystem::create_directories(directory);

            logger->info("Processing GGAK space environment data...");

            decodeFrames();
            logSummary();
            
            for (int i = 0; i < STATUS_COUNT; i++)
                inst_status[i] = DONE;

            GGAKProduct ggak_product;
            buildProduct(ggak_product);
            ggak_product.set_product_source(reader.sat_profile.name);
            ggak_product.set_product_timestamp(time(0));
            ggak_product.save(directory);

            satdump::products::DataSet dataset;
            dataset.satellite_name = reader.sat_profile.name;
            dataset.timestamp = time(0);
            dataset.products_list.push_back("GGAK");
            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));

            logger->info("GGAK processing complete.");
        }
    } // namespace ggak
} // namespace elektro_arktika
