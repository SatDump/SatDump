#pragma once
#include <hdf5.h>
#include <string>

namespace nc2pro
{
    int hdf5_get_int(hid_t &file, std::string path);
    float hdf5_get_float_attr(hid_t &file, std::string path, std::string attr);
    float hdf5_get_float(hid_t &file, std::string path);
    double hdf5_get_double_attr_FILE(hid_t &file, std::string attr);
    std::string hdf5_get_string_attr_FILE(hid_t &file, std::string attr);
    std::string hdf5_get_string_attr_FILE_fixed(hid_t &file, std::string attr);
}