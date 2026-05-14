#pragma once

#include "logger.h"
#include <H5Cpp.h>

namespace satdump
{
    namespace firstparty
    {
        inline std::string hdfpp_read_attribute_string(H5::Attribute attr)
        {
            std::string variable;
            attr.read(attr.getStrType(), variable);
            return variable;
        }

        inline float hdfpp_read_attribute_float(H5::Attribute attr)
        {
            float variable;
            attr.read(H5::PredType::NATIVE_FLOAT, &variable);
            return variable;
        }

        inline double hdfpp_read_attribute_double(H5::Attribute attr)
        {
            double variable;
            attr.read(H5::PredType::NATIVE_DOUBLE, &variable);
            return variable;
        }

        inline int hdfpp_read_attribute_int(H5::Attribute attr)
        {
            int variable;
            attr.read(H5::PredType::NATIVE_INT32, &variable);
            return variable;
        }

        inline int get_one_int_dataset(H5::H5File &file, std::string path)
        {
            H5::DataSet s = file.openDataSet(path);

            int val = 0;
            s.read(&val, H5::PredType::NATIVE_INT32);

            return val;
        }

        inline double get_one_double_dataset(H5::H5File &file, std::string path)
        {
            H5::DataSet s = file.openDataSet(path);

            double val = 0;
            s.read(&val, H5::PredType::NATIVE_DOUBLE);

            return val;
        }
    } // namespace firstparty
} // namespace satdump