#pragma once

#include "products.h"
#include "common/image/image.h"
#include "dll_export.h"

namespace satdump
{
    class ScatterometerProducts : public Products
    {
    public:
        // All channels, raw coefficients stored as
        // floats (yet to see a scat more precise than that)
        // std::map<int, std::vector<std::vector<float>>> scat_data;

        bool has_timestamps = true;

        // Few scat types... With some only aimed at a single instrument
        // due to how specific they are!
        SATDUMP_DLL static const std::string SCAT_TYPE_ASCAT;   // MetOp-A/B/C ASCAT, with its "unusual" 6 POVs. Theorically a normal pencil-beam, but as it's processed in a different way...
        SATDUMP_DLL static const std::string SCAT_TYPE_FANBEAM; // "Standard" Fanbeam scat, as seen on CFOSAT and others

        ///////////////////////// Data
        void set_scatterometer_type(std::string type)
        {
            contents["scat_type"] = type;
        }

        std::string get_scatterometer_type()
        {
            return contents["scat_type"];
        }

        void set_channel(int channel, std::vector<std::vector<float>> data)
        {
            // scat_data.insert_or_assign(channel, data);
            contents["data"][channel] = data;
        }

        std::vector<std::vector<float>> get_channel(int channel)
        {
            // return scat_data[channel];
            return contents["data"][channel];
        }

        int get_channel_cnt()
        {
            return contents["data"].size();
        }

        ///////////////////////// Timestamps

        void set_timestamps(int channel, std::vector<double> timestamps)
        {
            contents["timestamps"][channel] = timestamps;
        }

        std::vector<double> get_timestamps(int channel)
        {
            std::vector<double> timestamps;
            try
            {
                timestamps = contents["timestamps"][channel].get<std::vector<double>>();
            }
            catch (std::exception &)
            {
                timestamps = contents["timestamps"].get<std::vector<double>>();
            }

            return timestamps;
        }

        ///////////////////////// Projection

        void set_proj_cfg(int channel, nlohmann::ordered_json cfg)
        {
            contents["projection_cfg"][channel] = cfg;
        }

        nlohmann::ordered_json get_proj_cfg(int channel)
        {
            return contents["projection_cfg"][channel];
        }

        bool has_proj_cfg()
        {
            return contents.contains("projection_cfg");
        }

    public:
        virtual void save(std::string directory);
        virtual void load(std::string file);
    };

    // Map handling
    struct GrayScaleScatCfg
    {
        int channel; // On ASCAT, if in projs mode, this will be backward, down, forward instead of normal channels
        int min;
        int max;
    };

    inline void to_json(nlohmann::json &j, const GrayScaleScatCfg &v)
    {
        j["channel"] = v.channel;
        j["min"] = v.min;
        j["max"] = v.max;
    }

    inline void from_json(const nlohmann::json &j, GrayScaleScatCfg &v)
    {
        v.channel = j["channel"].get<int>();
        v.min = j["min"].get<int>();
        v.max = j["max"].get<int>();
    }

    image::Image make_scatterometer_grayscale(ScatterometerProducts &products, GrayScaleScatCfg cfg, float *progress = nullptr);
    image::Image make_scatterometer_grayscale_projs(ScatterometerProducts &products, GrayScaleScatCfg cfg, float *progress = nullptr, nlohmann::json *proj_cfg = nullptr);
}