#include "hdf5_utils.h"

namespace nc2pro
{
    std::string hdf5_get_string_attr_FILE_fixed(hid_t &file, std::string attr)
    {
        std::string val;

        if (file < 0)
            return "";
        hid_t att = H5Aopen(file, attr.c_str(), H5P_DEFAULT);
        hid_t att_type = H5Aget_type(att);
        char str[10000] = { 0 };
        H5Aread(att, att_type, &str);
        val = std::string(str);
        H5Tclose(att_type);
        H5Aclose(att);
        return val;
    }

	int hdf5_get_int(hid_t &file, std::string path)
    {
        int32_t val = 0;
        hid_t dataset = H5Dopen1(file, path.c_str());
        if (dataset < 0)
            return -1e6;
        H5Dread(dataset, H5T_NATIVE_INT32, H5S_ALL, H5S_ALL, H5P_DEFAULT, &val);
        H5Dclose(dataset);
        return val;
    }

    float hdf5_get_float(hid_t &file, std::string path)
    {
        float val = 0.0;
        hid_t dataset = H5Dopen1(file, path.c_str());
        if (dataset < 0)
            return -1e6;
        H5Dread(dataset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &val);
        H5Dclose(dataset);
        return val;
    }

    float hdf5_get_float_attr(hid_t &file, std::string path, std::string attr)
    {
        float val = -1e6;
        hid_t dataset = H5Dopen1(file, path.c_str());
        if (dataset < 0)
            return -1e6;
        hid_t att = H5Aopen(dataset, attr.c_str(), H5P_DEFAULT);
        H5Aread(att, H5T_NATIVE_FLOAT, &val);
        H5Aclose(att);
        H5Dclose(dataset);
        return val;
    }

    double hdf5_get_double_attr_FILE(hid_t& file, std::string attr)
    {
        double val = 0.0;
        if (file < 0)
            return -1e6;

        hid_t att = H5Aopen(file, attr.c_str(), H5P_DEFAULT);
        hid_t att_type = H5Aget_type(att);
        H5Aread(att, att_type, &val);
        H5Tclose(att_type);
        H5Aclose(att);
        return val;
    }

    std::string hdf5_get_string_attr_FILE(hid_t &file, std::string attr)
    {
        std::string val;

        if (file < 0)
            return "";
        hid_t att = H5Aopen(file, attr.c_str(), H5P_DEFAULT);
        hid_t att_type = H5Aget_type(att);
        char *str;
        H5Aread(att, att_type, &str);
        val = std::string(str);
        H5free_memory(str);
        H5Tclose(att_type);
        H5Aclose(att);
        return val;
    }
}