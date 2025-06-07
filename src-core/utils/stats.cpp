#include "stats.h"

namespace satdump
{
    double get_median(std::vector<double> values)
    {
        if (values.size() == 0)
            return 0;
        std::sort(values.begin(), values.end());
        size_t middle = values.size() / 2;
        return values[middle];
    }
} // namespace satdump