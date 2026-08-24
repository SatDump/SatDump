#pragma once

#include "elektro_arktika/ggak/skl.h"
#include "image/image.h"
#include <string>
#include <vector>

namespace elektro_arktika
{
    namespace ggak
    {
        image::Image plotData(std::string title, std::vector<double> time, std::vector<double> data, double time_start, double time_stop);

        std::vector<image::Image> plotSKLData(std::string satellite, double time_start, double time_stop, std::vector<SKLRecord> skl_data);
    } // namespace ggak
} // namespace elektro_arktika