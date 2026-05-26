#pragma once

#include <string>
#include <vector>

namespace satdump
{
    namespace ndsp
    {
        struct DSPBenchResult
        {
            std::string id;
            double throughput;
        };

        std::vector<std::string> getBenchCategories();
        std::vector<DSPBenchResult> runBenchmarks(std::vector<std::string> cats = {});
    } // namespace ndsp
} // namespace satdump