/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "init.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_utils.h"
#include <iostream>

std::string parseValue(std::ifstream &input_bin, std::string datatype, int typesize)
{
    std::vector<uint8_t> tmp_buf(typesize);
    input_bin.read((char *)tmp_buf.data(), tmp_buf.size());
    if (datatype == "int8_t")
        return std::to_string(*((int8_t *)&tmp_buf[0]));
    else if (datatype == "uint8_t")
        return std::to_string(*((uint8_t *)&tmp_buf[0]));
    else if (datatype == "int16_t")
        return std::to_string(*((int16_t *)&tmp_buf[0]));
    else if (datatype == "int32_t")
        return std::to_string(*((int32_t *)&tmp_buf[0]));
    else if (datatype == "float")
        return std::to_string(*((float *)&tmp_buf[0]));
    else if (datatype == "double")
        return std::to_string(*((double *)&tmp_buf[0]));
    printf("UKN DATA TYPE %s\n", datatype.c_str());
    return "";
}

std::string parseArray(std::ifstream &input_bin, std::string datatype, int typesize, int depth, int dim_sizes[], int curr_dim = 0)
{
    std::string final_decl;
    final_decl += "{";
    for (int i = 0; i < dim_sizes[curr_dim]; i++)
    {
        if (depth > 1)
            final_decl += parseArray(input_bin, datatype, typesize, depth - 1, dim_sizes, curr_dim + 1);
        else
            final_decl += parseValue(input_bin, datatype, typesize);
        if (i + 1 < dim_sizes[curr_dim])
            final_decl += ", ";
    }
    final_decl += "}";
    return final_decl;
}

int main(int argc, char *argv[])
{
    initLogger();

    nlohmann::json data_config = loadJsonFile(argv[1]);
    auto data_fields = data_config["DataProduct"]["ProductData"][0]["Field"];
    std::string struct_name = data_config["DataProduct"]["CollectionShortName"];
    for (char &c : struct_name)
        if (c == '-')
            c = '_';

    std::map<std::string, std::string> all_defines;
    std::string full_variables;

    std::ifstream input_bin(argv[2], std::ios::binary);

    if (std::stoi(data_config["DataProduct"]["ProductData"][0]["NumberOfFields"].get<std::string>()) == 1)
        data_fields = {data_fields};

    for (auto field : data_fields)
    {
        printf("%s\n", data_fields.dump().c_str());

        std::string symbol = field["Symbol"];
        std::string datatype = field["DataType"];
        int typesize = std::stoi(field["DataSize"]["Count"].get<std::string>());
        int numberofdims = std::stoi(field["NumberOfDimensions"].get<std::string>());

        if (datatype == "Float32")
            datatype = "float";
        else if (datatype == "Float64")
            datatype = "double";
        else if (datatype == "Int32")
            datatype = "int32_t";
        else if (datatype == "Int8")
            datatype = "int8_t";
        else if (datatype == "UInt8")
            datatype = "uint8_t";
        else if (datatype == "Int16")
            datatype = "int16_t";

        std::string final_decl = datatype + " " + symbol;

        if (numberofdims > 0)
        {
            int dims_size[100];
            try
            {
                int i = 0;
                for (auto dim : field["Dimension"])
                {
                    std::string dim_symbol = dim["Symbol"];
                    int size = std::stoi(dim["MaxIndex"].get<std::string>());

                    final_decl += "[" + dim_symbol + "]";

                    // printf("#define %s %d\n", dim_symbol.c_str(), size);

                    if (all_defines.count(dim_symbol) == 0)
                        all_defines.insert({dim_symbol, std::to_string(size)});

                    dims_size[i++] = size;
                }
            }
            catch (std::exception &e)
            {
                auto dim = field["Dimension"];

                std::string dim_symbol = dim["Symbol"];
                int size = std::stoi(dim["MaxIndex"].get<std::string>());

                final_decl += "[" + dim_symbol + "]";

                // printf("#define %s %d\n", dim_symbol.c_str(), size);

                if (all_defines.count(dim_symbol) == 0)
                    all_defines.insert({dim_symbol, std::to_string(size)});

                dims_size[0] = size;
            }

            final_decl += " = " + parseArray(input_bin, datatype, typesize, numberofdims, dims_size);
        }
        else
        {
            final_decl += " = " + parseValue(input_bin, datatype, typesize);
        }

        // printf("%s;\n\n", final_decl.c_str());
        full_variables += "    " + final_decl + ";\n";
    }

    // printf("#pragma once\n\n");
    printf("struct %s\n{\n", struct_name.c_str());
    for (auto def : all_defines)
        printf("    static const int %s = %s;\n", def.first.c_str(), def.second.c_str());
    printf("\n");
    printf("%s", full_variables.c_str());
    printf("};\n");
}