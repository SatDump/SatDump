#pragma once

#include <string>

namespace hsd2pro
{
    void process_himawari_ahi(std::string hsd_file, std::string pro_output_file, double *progress = nullptr);
}
