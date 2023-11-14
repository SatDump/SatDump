#pragma once

#include "products.h"
#include "common/image/image.h"
#include <mutex>

#define CALIBRATION_INVALID_VALUE -999.99

namespace satdump
{
    class ImageProducts : public Products
    {
    public:
        struct ImageHolder
        {
            std::string filename;
            std::string channel_name;
            image::Image<uint16_t> image;
            std::vector<double> timestamps = std::vector<double>();
            int ifov_y = -1;
            int ifov_x = -1;
            int offset_x = 0;
        };

        std::vector<ImageHolder> images;

        bool has_timestamps = true;
        bool needs_correlation = false;
        int ifov_y = -1;
        int ifov_x = -1;

        int bit_depth = 16;

        bool save_as_matrix = false;

        ///////////////////////// Timestamps

        enum Timestamp_Type
        {
            TIMESTAMP_LINE,
            TIMESTAMP_MULTIPLE_LINES,
            TIMESTAMP_IFOV,
        };

        Timestamp_Type timestamp_type;

        void set_timestamps(std::vector<double> timestamps)
        {
            contents["timestamps"] = timestamps;
        }

        std::vector<double> get_timestamps(int image_index = -1)
        {
            try
            {
                if (image_index == -1)
                    return contents["timestamps"].get<std::vector<double>>();

                if ((int)images.size() > image_index)
                {
                    if (images[image_index].timestamps.size() > 0)
                        return images[image_index].timestamps;
                    else
                        return contents["timestamps"].get<std::vector<double>>();
                }
                else
                    return contents["timestamps"].get<std::vector<double>>();
            }
            catch (std::exception &)
            {
                return {};
            }
        }

        int get_ifov_y_size(int image_index = -1)
        {
            if (images[image_index].ifov_y != -1)
                return images[image_index].ifov_y;
            else
                return ifov_y;
        }

        int get_ifov_x_size(int image_index = -1)
        {
            if (images[image_index].ifov_x != -1)
                return images[image_index].ifov_x;
            else
                return ifov_x;
        }

        ///////////////////////// Projection

        void set_proj_cfg(nlohmann::ordered_json cfg)
        {
            contents["projection_cfg"] = cfg;
        }

        nlohmann::ordered_json get_proj_cfg()
        {
            return contents["projection_cfg"];
        }

        nlohmann::ordered_json get_channel_proj_metdata(int ch)
        {
            nlohmann::ordered_json mtd;
            if (images[ch].offset_x != 0)
                mtd["img_offset_x"] = images[ch].offset_x;
            return mtd;
        }

        bool has_proj_cfg()
        {
            return contents.contains("projection_cfg");
        }

        bool can_geometrically_correct()
        {
            if (!has_proj_cfg())
                return false;
            if (!contents.contains("projection_cfg"))
                return false;
            if (!get_proj_cfg().contains("corr_swath"))
                return false;
            if (!get_proj_cfg().contains("corr_resol"))
                return false;
            if (!get_proj_cfg().contains("corr_altit"))
                return false;
            return true;
        }

        /// CALIBRATION

        enum calib_type_t
        {
            CALIB_REFLECTANCE,
            CALIB_RADIANCE,
        };

        enum calib_vtype_t
        {
            CALIB_VTYPE_AUTO,
            CALIB_VTYPE_ALBEDO,
            CALIB_VTYPE_RADIANCE,
            CALIB_VTYPE_TEMPERATURE,
        };

        class CalibratorBase
        {
        public:
            const nlohmann::json d_calib;
            ImageProducts *d_products;

        public:
            CalibratorBase(nlohmann::json calib, ImageProducts *products) : d_calib(calib), d_products(products) {}
            ~CalibratorBase() {}
            virtual void init() = 0;
            virtual double compute(int image_index, int x, int y, int val) = 0;
        };

        struct RequestCalibratorEvent
        {
            std::string id;
            std::vector<std::shared_ptr<CalibratorBase>> &calibrators;
            nlohmann::json calib;
            ImageProducts *products;
        };

        bool has_calibation()
        {
            return contents.contains("calibration");
        }

        void init_calibration();

        void set_calibration(nlohmann::json calib)
        {
            bool d = false;
            nlohmann::json buff;
            if (has_calibation() && contents["calibration"].contains("wavenumbers"))
            {
                buff = contents["calibration"]["wavenumbers"];
                d = true;
            }
            contents["calibration"] = calib;
            if (d)
                contents["calibration"]["wavenumbers"] = buff;
        }

        double get_wavenumber(int image_index)
        {
            if (!has_calibation())
                return 0;

            if (contents["calibration"].contains("wavenumbers"))
                return contents["calibration"]["wavenumbers"][image_index].get<double>();
            else
                return 0;
        }

        void set_wavenumber(int image_index, double wavnb)
        {
            contents["calibration"]["wavenumbers"][image_index] = wavnb;
        }

        void set_calibration_type(int image_index, calib_type_t ct)
        {
            contents["calibration"]["type"][image_index] = (int)ct;
        }

        void set_calibration_default_radiance_range(int image_index, double rad_min, double rad_max)
        {
            contents["calibration"]["default_range"][image_index]["min"] = rad_min;
            contents["calibration"]["default_range"][image_index]["max"] = rad_max;
        }

        std::pair<double, double> get_calibration_default_radiance_range(int image_index)
        {
            if (!has_calibation())
                return {0, 0};
            if (contents["calibration"].contains("default_range"))
                return {contents["calibration"]["default_range"][image_index]["min"].get<double>(), contents["calibration"]["default_range"][image_index]["max"].get<double>()};
            if (get_calibration_type(image_index) == CALIB_REFLECTANCE)
                return {0, 1};
            return {0, 0};
        }

        calib_type_t get_calibration_type(int image_index)
        {
            if (!has_calibation())
                return CALIB_REFLECTANCE;
            if (contents["calibration"].contains("type"))
                return (calib_type_t)contents["calibration"]["type"][image_index].get<int>();
            else
                return CALIB_REFLECTANCE;
        }

        double get_calibrated_value(int image_index, int x, int y, bool temp = false);

        image::Image<uint16_t> get_calibrated_image(int image_index, float *progress = nullptr, calib_vtype_t vtype = CALIB_VTYPE_AUTO, std::pair<double, double> range = {0, 0});

    public:
        virtual void save(std::string directory);
        virtual void load(std::string file);

        ~ImageProducts();

    private:
        std::map<int, image::Image<uint16_t>> calibrated_img_cache;
        std::mutex calib_mutex;
        std::shared_ptr<CalibratorBase> calibrator_ptr = nullptr;
        void *lua_state_ptr = nullptr; // Opaque pointer to not include sol2 here... As it's big!
        void *lua_comp_func_ptr = nullptr;
    };

    // Composite handling
    struct ImageCompositeCfg
    {
        std::string equation;
        bool despeckle = false;
        bool equalize = false;
        bool individual_equalize = false;
        bool invert = false;
        bool normalize = false;
        bool white_balance = false;
        bool apply_lut = false;

        std::string lut = "";
        std::string channels = "";
        std::string lua = "";
        nlohmann::json lua_vars;
        nlohmann::json calib_cfg;

        std::string description_markdown = "";
    };

    inline void to_json(nlohmann::json &j, const ImageCompositeCfg &v)
    {
        j["equation"] = v.equation;
        j["despeckle"] = v.despeckle;
        j["equalize"] = v.equalize;
        j["individual_equalize"] = v.individual_equalize;
        j["invert"] = v.invert;
        j["normalize"] = v.normalize;
        j["white_balance"] = v.white_balance;
        j["apply_lut"] = v.apply_lut;

        j["lut"] = v.lut;
        j["channels"] = v.channels;
        j["lua"] = v.lua;
        j["lua_vars"] = v.lua_vars;

        j["calib_cfg"] = v.calib_cfg;
    }

    inline void from_json(const nlohmann::json &j, ImageCompositeCfg &v)
    {
        if (j.contains("equation"))
        {
            v.equation = j["equation"].get<std::string>();
        }
        else if (j.contains("lut"))
        {
            v.lut = j["lut"].get<std::string>();
            v.channels = j["channels"].get<std::string>();
        }
        else if (j.contains("lua"))
        {
            v.lua = j["lua"].get<std::string>();
            v.channels = j["channels"].get<std::string>();
            if (j.contains("lua_vars"))
                v.lua_vars = j["lua_vars"];
        }

        if (j.contains("calib_cfg"))
            v.calib_cfg = j["calib_cfg"];

        if (j.contains("despeckle"))
            v.despeckle = j["despeckle"].get<bool>();
        if (j.contains("equalize"))
            v.equalize = j["equalize"].get<bool>();
        if (j.contains("individual_equalize"))
            v.individual_equalize = j["individual_equalize"].get<bool>();
        if (j.contains("invert"))
            v.invert = j["invert"].get<bool>();
        if (j.contains("normalize"))
            v.normalize = j["normalize"].get<bool>();
        if (j.contains("white_balance"))
            v.white_balance = j["white_balance"].get<bool>();
        if (j.contains("apply_lut"))
            v.apply_lut = j["apply_lut"].get<bool>();

        if (j.contains("description"))
            v.description_markdown = j["description"].get<std::string>();
    }

    image::Image<uint16_t> make_composite_from_product(ImageProducts &product, ImageCompositeCfg cfg, float *progress = nullptr, std::vector<double> *final_timestamps = nullptr, nlohmann::json *final_metadata = nullptr);
    image::Image<uint16_t> perform_geometric_correction(ImageProducts &product, image::Image<uint16_t> img, bool &success, float *foward_table = nullptr);

    std::vector<int> generate_horizontal_corr_lut(ImageProducts &product, int width);
}