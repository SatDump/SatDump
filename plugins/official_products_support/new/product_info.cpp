#include "product_info.h"
#include "core/exception.h"
#include "logger.h"
#include "utils/time.h"
#include <cmath>
#include <cstring>
#include <ctime>

#include "processors/nat/msg/seviri_nat.h"
#include "processors/nc/goes/abi_nc.h"
#include "processors/nc/mtg/fci_nc.h"

namespace satdump
{
    namespace official
    {
        // TODOREWORK move!
        std::string MetOpNumToName(int num)
        {
            if (num == 1)
                return "MetOp-B";
            else if (num == 2)
                return "MetOp-A";
            else if (num == 3)
                return "MetOp-C";
            return "MetOp-Invalid";
        }

        std::vector<RegisteredOfficialProduct> getRegisteredProducts()
        {
            return {
                {NATIVE_MSG_SEVIRI,
                 [](std::unique_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     OfficialProductInfo i;

                     int sat_num, len;
                     std::tm timeS;
                     memset(&timeS, 0, sizeof(std::tm));
                     if (sscanf(f->name.c_str(), "MSG%d-SEVI-MSG15-%*d-NA-%4d%2d%2d%2d%2d%2d.%*dZ-NA.nat%n", &sat_num, &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min,
                                &timeS.tm_sec, &len) == 7 &&
                         len == f->name.size())
                     {
                         i.type = NATIVE_MSG_SEVIRI;

                         timeS.tm_year -= 1900;
                         timeS.tm_mon -= 1;
                         i.timestamp = timegm(&timeS);

                         i.name = "MSG-" + std::to_string(sat_num) + " SEVIRI " + timestamp_to_string(i.timestamp);
                     }

                     return i;
                 },
                 []() { return std::make_shared<SEVIRINatProcessor>(); }},

                {NATIVE_METOP_AVHRR, // TODOREWORK
                 [](std::unique_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     OfficialProductInfo i;

                     int sat_num, len;
                     std::tm timeS;
                     memset(&timeS, 0, sizeof(std::tm));
                     if (sscanf(f->name.c_str(), "AVHR_xxx_1B_M%2d_%4d%2d%2d%2d%2d%2dZ_%*dZ_N_O_%*dZ.nat%n", &sat_num, &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min,
                                &timeS.tm_sec, &len) == 7 &&
                         len == f->name.size())
                     {
                         i.type = NATIVE_METOP_AVHRR;

                         timeS.tm_year -= 1900;
                         timeS.tm_mon -= 1;
                         i.timestamp = timegm(&timeS);

                         i.name = MetOpNumToName(sat_num) + " AVHRR/3 " + timestamp_to_string(i.timestamp);
                     }

                     return i;
                 }},

                {NETCDF_MTG_FCI,
                 [](std::unique_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     OfficialProductInfo i;

                     int sat_num, len;
                     std::tm timeS;
                     memset(&timeS, 0, sizeof(std::tm));
                     uint32_t test1, test2;
                     int day_repeat_cycle;
                     if (sscanf(f->name.c_str(), "W_XX-EUMETSAT-Darmstadt,IMG+SAT,MTI%d+FCI-1C-RRAD-FDHSI-FD--CHK-BODY---NC4E_C_EUMT_%4d%2d%2d%2d%2d%2d_IDPFI_OPE_%14u_%14u_N__O_%4d_%*d.nc%n",
                                &sat_num, &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec, &test1, &test2, &day_repeat_cycle, &len) == 10 &&
                         len == f->name.size())
                     {
                         i.type = NETCDF_MTG_FCI;

                         timeS.tm_year -= 1900;
                         timeS.tm_mon -= 1;
                         i.timestamp = timegm(&timeS);

                         i.name = "MTG-I" + std::to_string(sat_num) + " FCI " + timestamp_to_string(i.timestamp);

                         i.group_id = std::to_string((uint64_t)floor(i.timestamp / (24 * 3600))) + "_" + std::to_string(day_repeat_cycle);
                     }

                     return i;
                 },
                 []() { return std::make_shared<FCINcProcessor>(); }},

                {NETCDF_GOES_ABI,
                 [](std::unique_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     OfficialProductInfo i;

                     int sat_num, day_of_year, len;
                     std::tm timeS;
                     memset(&timeS, 0, sizeof(std::tm));
                     uint32_t test1, test2;
                     if (sscanf(f->name.c_str(), "OR_ABI-L1b-RadF-M%*dC%*d_G%d_s%4d%3d%2d%2d%2d%*d_e%14d_c%14d.nc%n", &sat_num, &timeS.tm_year, &day_of_year, &timeS.tm_hour, &timeS.tm_min,
                                &timeS.tm_sec, &test1, &test2, &len) == 8 &&
                         len == f->name.size())
                     {
                         i.type = NETCDF_GOES_ABI;

                         timeS.tm_year -= 1900;
                         i.timestamp = timegm(&timeS) + day_of_year * 3600 * 24;

                         i.name = "GOES-" + std::to_string(sat_num) + " ABI " + timestamp_to_string(i.timestamp);

                         i.group_id = std::to_string((time_t)i.timestamp);
                     }

                     return i;
                 },
                 []() { return std::make_shared<ABINcProcessor>(); }},
            };
        }

        OfficialProductInfo parseOfficialInfo(std::unique_ptr<satdump::utils::FilesIteratorItem> &f)
        {
            for (auto &p : getRegisteredProducts())
            {
                auto finfo = p.testFile(f);
                if (finfo.type != PRODUCT_NONE)
                    return finfo;
            }
            return OfficialProductInfo();
        }

        std::shared_ptr<OfficialProductProcessor> getProcessorForProduct(OfficialProductInfo info)
        {
            for (auto &p : getRegisteredProducts())
            {
                if (p.type == info.type)
                    return p.getProcessor();
            }
            throw satdump_exception("No processor found for " + info.name + "!");
        }
    } // namespace official
} // namespace satdump