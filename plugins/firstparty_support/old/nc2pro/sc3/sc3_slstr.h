#pragma once

#include <string>
#include <vector>

namespace nc2pro
{
    void process_sc3_slstr(std::string zip_file, std::string pro_output_file, std::vector<std::string> &product_paths, double *progess);
}
