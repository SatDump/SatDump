#include "ggak_product.h"
#include "logger.h"

#include <algorithm>
#include <filesystem>

namespace elektro_arktika
{
    namespace ggak
    {
        GGAKChannel with_gap_markers(const GGAKChannel &src)
        {
            GGAKChannel ch = src;
            if (ch.timestamps.size() < 2)
                return ch;

            std::vector<double> deltas;
            deltas.reserve(ch.timestamps.size());
            for (size_t i = 1; i < ch.timestamps.size(); i++)
            {
                double dt = ch.timestamps[i] - ch.timestamps[i - 1];
                if (dt > 0.01)
                    deltas.push_back(dt);
            }
            if (deltas.empty())
                return ch;

            std::sort(deltas.begin(), deltas.end());
            double cadence = deltas[deltas.size() / 2];
            double gap_threshold = 1.5 * cadence;

            std::vector<double> new_ts, new_vals;
            new_ts.reserve(ch.timestamps.size() * 2);
            new_vals.reserve(new_ts.capacity());

            new_ts.push_back(ch.timestamps[0]);
            new_vals.push_back(ch.values[0]);

            for (size_t i = 1; i < ch.timestamps.size(); i++)
            {
                double dt = ch.timestamps[i] - ch.timestamps[i - 1];
                if (dt > gap_threshold)
                {
                    double nan = std::numeric_limits<double>::quiet_NaN();
                    new_ts.push_back(nan);
                    new_vals.push_back(nan);
                }
                new_ts.push_back(ch.timestamps[i]);
                new_vals.push_back(ch.values[i]);
            }

            ch.timestamps = std::move(new_ts);
            ch.values = std::move(new_vals);
            return ch;
        }

        std::vector<GGAKChannel> with_gap_markers(const std::vector<GGAKChannel> &src)
        {
            std::vector<GGAKChannel> out;
            out.reserve(src.size());
            for (const auto &ch : src)
                out.push_back(with_gap_markers(ch));
            return out;
        }

        void GGAKProduct::save(std::string directory)
        {
            auto warn_inconsistent = [](const std::vector<GGAKChannel> &channels, const char *group) {
                for (auto &ch : channels)
                    if (!ch.isConsistent())
                        logger->warn("GGAKProduct::save: channel '%s' in %s has mismatched sizes (ts=%zu, vals=%zu)",
                                     ch.name.c_str(), group, ch.timestamps.size(), ch.values.size());
            };
            warn_inconsistent(mag_channels, "mag_channels");
            warn_inconsistent(particle_channels, "particle_channels");
            warn_inconsistent(subpacket_channels, "subpacket_channels");

            contents["mag_channels"] = mag_channels;
            contents["particle_channels"] = particle_channels;
            contents["subpacket_channels"] = subpacket_channels;
            contents["stats"] = stats;
            Product::save(directory);
        }

        void GGAKProduct::load(std::string file)
        {
            Product::load(file);

            try
            {
                if (contents.contains("mag_channels"))
                    mag_channels = contents["mag_channels"].get<std::vector<GGAKChannel>>();
                if (contents.contains("particle_channels"))
                    particle_channels = contents["particle_channels"].get<std::vector<GGAKChannel>>();
                if (contents.contains("subpacket_channels"))
                    subpacket_channels = contents["subpacket_channels"].get<std::vector<GGAKChannel>>();

                if (contents.contains("stats"))
                    stats = contents["stats"].get<GGAKFrameStats>();
            }
            catch (const std::exception &e)
            {
                logger->error("GGAKProduct::load failed to parse: %s", e.what());
                mag_channels.clear();
                particle_channels.clear();
                subpacket_channels.clear();
                stats = {};
            }
        }
    } // namespace ggak
} // namespace elektro_arktika
