#pragma once

#include "products.h"
#include "common/image/image.h"

namespace satdump
{
    class RadiationProducts : public Products
    {
    public:
        std::vector<std::vector<int>> channel_counts;
        bool has_timestamps = true;

        void set_timestamps(int channel, std::vector<double> timestamps)
        {
            contents["timestamps"][channel] = timestamps;
        }

        void set_timestamps_all(std::vector<double> timestamps)
        {
            contents["timestamps"] = timestamps;
        }

        std::vector<double> get_timestamps(int channel)
        {
            std::vector<double> timestamps;
            try
            {
                timestamps = contents["timestamps"][channel].get<std::vector<double>>();
            }
            catch (std::exception &e)
            {
                timestamps = contents["timestamps"].get<std::vector<double>>();
            }

            return timestamps;
        }

        void set_energy(int channel, double energy)
        {
            contents["energy"][channel] = energy;
        }

        double get_energy(int channel)
        {
            if (contents.contains("energy"))
                return contents["energy"][channel].get<double>();
            else
                return 0;
        }

    public:
        virtual void save(std::string directory);
        virtual void load(std::string file);
    };

    void make_radiation_map(RadiationProducts &products, int channel, image::Image<uint16_t> &map, int radius, image::Image<uint16_t> color_lut, int min, int max);
}