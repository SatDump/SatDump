#include "ggak_product.h"
#include "logger.h"

#include <filesystem>
#include <fstream>

namespace elektro_arktika
{
    namespace ggak
    {
        namespace
        {
            void save_channels_binary(const std::string &path,
                                      const std::vector<GGAKChannel> &channels)
            {
                std::ofstream f(path, std::ios::binary);
                if (!f.is_open())
                    return;
                uint32_t n_ch = static_cast<uint32_t>(channels.size());
                f.write(reinterpret_cast<const char *>(&n_ch), sizeof(n_ch));
                for (auto &ch : channels)
                {
                    uint32_t n = static_cast<uint32_t>(ch.values.size());
                    f.write(reinterpret_cast<const char *>(&n), sizeof(n));
                    f.write(reinterpret_cast<const char *>(ch.timestamps.data()),
                            n * sizeof(double));
                    f.write(reinterpret_cast<const char *>(ch.values.data()),
                            n * sizeof(double));
                }
            }

            std::vector<GGAKChannel> load_channels_binary(const std::string &path,
                                                           const nlohmann::json &meta)
            {
                std::vector<GGAKChannel> channels;
                std::ifstream f(path, std::ios::binary);
                if (!f.is_open())
                    return channels;

                uint32_t n_ch = 0;
                f.read(reinterpret_cast<char *>(&n_ch), sizeof(n_ch));

                for (uint32_t i = 0; i < n_ch && i < meta.size(); i++)
                {
                    GGAKChannel ch;
                    ch.name = meta[i]["name"].get<std::string>();
                    ch.unit = meta[i]["unit"].get<std::string>();
                    if (meta[i].contains("group"))
                        ch.group = meta[i]["group"].get<std::string>();

                    uint32_t n = 0;
                    f.read(reinterpret_cast<char *>(&n), sizeof(n));
                    ch.timestamps.resize(n);
                    ch.values.resize(n);
                    f.read(reinterpret_cast<char *>(ch.timestamps.data()),
                           n * sizeof(double));
                    f.read(reinterpret_cast<char *>(ch.values.data()),
                           n * sizeof(double));
                    ch.updateMinMax();
                    channels.push_back(std::move(ch));
                }
                return channels;
            }

            nlohmann::json make_channel_meta(const std::vector<GGAKChannel> &channels)
            {
                nlohmann::json j = nlohmann::json::array();
                for (auto &ch : channels)
                {
                    nlohmann::json jc;
                    jc["name"] = ch.name;
                    jc["unit"] = ch.unit;
                    if (!ch.group.empty())
                        jc["group"] = ch.group;
                    jc["count"] = ch.values.size();
                    j.push_back(jc);
                }
                return j;
            }
        } // anonymous namespace

        void GGAKProduct::save(std::string directory)
        {
            save_channels_binary(directory + "/mag.bin", mag_channels);
            save_channels_binary(directory + "/particles.bin", particle_channels);
            save_channels_binary(directory + "/subpackets.bin", subpacket_channels);

            contents["channel_format"] = "binary";
            contents["mag_channels"] = make_channel_meta(mag_channels);
            contents["particle_channels"] = make_channel_meta(particle_channels);
            contents["subpacket_channels"] = make_channel_meta(subpacket_channels);
            contents["stats"] = stats;
            Product::save(directory);
        }

        void GGAKProduct::load(std::string file)
        {
            Product::load(file);
            std::string directory = std::filesystem::path(file).parent_path().string();

            try
            {
                bool binary = contents.contains("channel_format") &&
                              contents["channel_format"] == "binary";

                if (binary)
                {
                    if (contents.contains("mag_channels"))
                        mag_channels = load_channels_binary(
                            directory + "/mag.bin", contents["mag_channels"]);
                    if (contents.contains("particle_channels"))
                        particle_channels = load_channels_binary(
                            directory + "/particles.bin", contents["particle_channels"]);
                    if (contents.contains("subpacket_channels"))
                        subpacket_channels = load_channels_binary(
                            directory + "/subpackets.bin", contents["subpacket_channels"]);
                }
                else
                {
                    if (contents.contains("mag_channels"))
                        mag_channels = contents["mag_channels"].get<std::vector<GGAKChannel>>();
                    if (contents.contains("particle_channels"))
                        particle_channels = contents["particle_channels"].get<std::vector<GGAKChannel>>();
                    if (contents.contains("subpacket_channels"))
                        subpacket_channels = contents["subpacket_channels"].get<std::vector<GGAKChannel>>();
                }

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
