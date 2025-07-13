#include "unit_parser.h"
#include "utils/string.h"
#include <algorithm>

namespace satdump
{
    bool parseUnitFromString(std::string str, double &out, std::vector<UnitInfo> unit)
    {
        // To avoid find() matching on smaller strings first
        std::sort(unit.begin(), unit.end(), [](const auto &lhs, const auto &rhs) { return lhs.name.size() > rhs.name.size(); });

        for (auto &u : unit)
        {
            if (str.find(u.name) != std::string::npos)
            {
                replaceAllStr(str, u.name, "");
                out = (std::stod(str) * u.scale);
                return true;
            }
        }

        return false;
    }
} // namespace satdump