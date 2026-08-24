#pragma once

#include "elektro_arktika/ggak/skl.h"
#include "ggak.h"
#include "image/image.h"
#include <string>
#include <vector>

namespace elektro_arktika
{
    namespace ggak
    {
        std::vector<SKLRecord> debug_parseSKLCSV(std::string csv);

        image::Image debug_plotSKLSingle(std::vector<SKLRecord> results1, int ch, bool off, double time_start, double time_stop);
        image::Image debug_plotSKL(std::vector<SKLRecord> results1, std::vector<SKLRecord> results2, int ch, double time_start, double time_stop);
    } // namespace ggak
} // namespace elektro_arktika