#pragma once

/**
 * @file ggak_product.h
 * @brief GGAK product data model for the SatDump product system.
 *
 * Defines the channel data structure (GGAKChannel), frame statistics
 * (GGAKFrameStats), and the GGAKProduct class that extends the core
 * Product with magnetometer, particle, and spectrometer time series.
 */

#include "products/product.h"
#include "nlohmann/json.hpp"
#include <cmath>
#include <limits>
#include <map>
#include <vector>

namespace elektro_arktika
{
    namespace ggak
    {
        struct GGAKChannel
        {
            std::string name;
            std::string unit;
            std::string group;
            std::vector<double> timestamps;
            std::vector<double> values;

            double cached_min = 0;
            double cached_max = 0;
            int cached_valid_count = 0;

            bool isConsistent() const
            {
                return timestamps.size() == values.size();
            }

            void updateMinMax()
            {
                cached_min = std::numeric_limits<double>::max();
                cached_max = -std::numeric_limits<double>::max();
                cached_valid_count = 0;
                size_t n = std::min(timestamps.size(), values.size());
                for (size_t i = 0; i < n; i++)
                {
                    if (std::isnan(values[i]))
                        continue;
                    cached_valid_count++;
                    if (values[i] < cached_min) cached_min = values[i];
                    if (values[i] > cached_max) cached_max = values[i];
                }
                if (cached_valid_count == 0)
                    cached_min = cached_max = 0;
            }

            friend void to_json(nlohmann::json &j, const GGAKChannel &c)
            {
                j = nlohmann::json{{"name", c.name}, {"unit", c.unit},
                                   {"timestamps", c.timestamps}, {"values", c.values}};
                if (!c.group.empty())
                    j["group"] = c.group;
            }

            friend void from_json(const nlohmann::json &j, GGAKChannel &c)
            {
                j.at("name").get_to(c.name);
                j.at("unit").get_to(c.unit);
                j.at("timestamps").get_to(c.timestamps);
                j.at("values").get_to(c.values);
                if (j.contains("group"))
                    j.at("group").get_to(c.group);
                c.updateMinMax();
            }
        };

        struct GGAKFrameStats
        {
            int total_frames = 0;
            int data_frames = 0;
            int fill_frames = 0;
            int checksum_pass = 0;
            int checksum_fail = 0;
            int counter_gaps = 0;
            int missing_estimate = 0;
            std::string generation;
            std::string satellite_name;
            uint8_t observed_source_id = 0;
            std::map<std::string, int> per_instrument_gaps;
            std::map<std::string, int> per_instrument_missing;

            friend void to_json(nlohmann::json &j, const GGAKFrameStats &s)
            {
                j = nlohmann::json{
                    {"total_frames", s.total_frames},
                    {"data_frames", s.data_frames},
                    {"fill_frames", s.fill_frames},
                    {"checksum_pass", s.checksum_pass},
                    {"checksum_fail", s.checksum_fail},
                    {"counter_gaps", s.counter_gaps},
                    {"missing_estimate", s.missing_estimate},
                    {"generation", s.generation},
                    {"satellite_name", s.satellite_name},
                    {"observed_source_id", s.observed_source_id}};
                if (!s.per_instrument_gaps.empty())
                    j["per_instrument_gaps"] = s.per_instrument_gaps;
                if (!s.per_instrument_missing.empty())
                    j["per_instrument_missing"] = s.per_instrument_missing;
            }

            friend void from_json(const nlohmann::json &j, GGAKFrameStats &s)
            {
                j.at("total_frames").get_to(s.total_frames);
                j.at("data_frames").get_to(s.data_frames);
                j.at("fill_frames").get_to(s.fill_frames);
                j.at("checksum_pass").get_to(s.checksum_pass);
                j.at("checksum_fail").get_to(s.checksum_fail);
                j.at("counter_gaps").get_to(s.counter_gaps);
                j.at("missing_estimate").get_to(s.missing_estimate);
                j.at("generation").get_to(s.generation);
                j.at("satellite_name").get_to(s.satellite_name);
                if (j.contains("observed_source_id"))
                    j.at("observed_source_id").get_to(s.observed_source_id);
                if (j.contains("per_instrument_gaps"))
                    j.at("per_instrument_gaps").get_to(s.per_instrument_gaps);
                if (j.contains("per_instrument_missing"))
                    j.at("per_instrument_missing").get_to(s.per_instrument_missing);
            }
        };

        class GGAKProduct : public satdump::products::Product
        {
        public:
            std::vector<GGAKChannel> mag_channels;
            std::vector<GGAKChannel> particle_channels;
            std::vector<GGAKChannel> subpacket_channels;
            GGAKFrameStats stats;

            GGAKProduct() { type = "ggak"; }

            void save(std::string directory);
            void load(std::string file);

            int totalChannels() const
            {
                return (int)(mag_channels.size() + particle_channels.size() + subpacket_channels.size());
            }

            int totalSamples() const
            {
                int n = 0;
                for (auto &c : mag_channels)
                    n += (int)c.values.size();
                for (auto &c : particle_channels)
                    n += (int)c.values.size();
                for (auto &c : subpacket_channels)
                    n += (int)c.values.size();
                return n;
            }
        };

        GGAKChannel with_gap_markers(const GGAKChannel &src);
        std::vector<GGAKChannel> with_gap_markers(const std::vector<GGAKChannel> &src);
    } // namespace ggak
} // namespace elektro_arktika
