#include "product_info.h"
#include "core/exception.h"
#include "logger.h"
#include "processors/hdf/dmsp/ssmis_hdf.h"
#include "processors/hdf/fy2/svissr_hdf.h"
#include "processors/hdf/fy3/mersi_hdf.h"
#include "processors/hdf/gpm/gmi_hdf.h"
#include "processors/hrit/hrit_generic.h"
#include "processors/hsd/himawari/ahi_hsd.h"
#include "processors/nat/metop/amsu_nat.h"
#include "processors/nat/metop/avhrr_nat.h"
#include "processors/nat/metop/hirs_nat.h"
#include "processors/nat/metop/iasi_nat.h"
#include "processors/nat/metop/mhs_nat.h"
#include "processors/nc/aws/mwr_nc.h"
#include "processors/nc/gk2a/ami_nc.h"
#include "type.h"
#include "utils/string.h"
#include "utils/time.h"
#include <cmath>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <string>

#include "processors/nat/msg/seviri_nat.h"
#include "processors/nc/goes/abi_nc.h"
#include "processors/nc/mtg/fci_nc.h"
#include "xrit/identify.h"

namespace satdump
{
    namespace firstparty
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

        std::vector<RegisteredFirstPartyProduct> getRegisteredProducts()
        {
            return {
                {NATIVE_MSG_SEVIRI,
                 [](std::shared_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     FirstPartyProductInfo i;

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

                {NATIVE_METOP_AVHRR,
                 [](std::shared_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     FirstPartyProductInfo i;

                     int sat_num, len;
                     std::tm timeS;
                     memset(&timeS, 0, sizeof(std::tm));
                     if ((sscanf(f->name.c_str(), "AVHR_xxx_1B_M%2d_%4d%2d%2d%2d%2d%2dZ_%*dZ_N_O_%*dZ.nat%n", &sat_num, &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min,
                                 &timeS.tm_sec, &len) == 7 &&
                          len == f->name.size()) ||
                         (sscanf(f->name.c_str(), "AVHR_xxx_1B_M%2d_%4d%2d%2d%2d%2d%2dZ_%*dZ_N_O_%*dZ.bz2%n", &sat_num, &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min,
                                 &timeS.tm_sec, &len) == 7 && // TODOREWORK the rest can be compressed too perhaps?
                          len == f->name.size()))
                     {
                         i.type = NATIVE_METOP_AVHRR;

                         timeS.tm_year -= 1900;
                         timeS.tm_mon -= 1;
                         i.timestamp = timegm(&timeS);

                         i.name = MetOpNumToName(sat_num) + " AVHRR/3 " + timestamp_to_string(i.timestamp);
                     }

                     return i;
                 },
                 []() { return std::make_shared<AVHRRNatProcessor>(); }},

                {NATIVE_METOP_MHS,
                 [](std::shared_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     FirstPartyProductInfo i;

                     int sat_num, len;
                     std::tm timeS;
                     memset(&timeS, 0, sizeof(std::tm));
                     if (sscanf(f->name.c_str(), "MHSx_xxx_1B_M%2d_%4d%2d%2d%2d%2d%2dZ_%*dZ_N_O_%*dZ.nat%n", &sat_num, &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min,
                                &timeS.tm_sec, &len) == 7 &&
                         len == f->name.size())
                     {
                         i.type = NATIVE_METOP_MHS;

                         timeS.tm_year -= 1900;
                         timeS.tm_mon -= 1;
                         i.timestamp = timegm(&timeS);

                         i.name = MetOpNumToName(sat_num) + " MHS " + timestamp_to_string(i.timestamp);
                     }

                     return i;
                 },
                 []() { return std::make_shared<MHSNatProcessor>(); }},

                {NATIVE_METOP_AMSUA,
                 [](std::shared_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     FirstPartyProductInfo i;

                     int sat_num, len;
                     std::tm timeS;
                     memset(&timeS, 0, sizeof(std::tm));
                     if (sscanf(f->name.c_str(), "AMSA_xxx_1B_M%2d_%4d%2d%2d%2d%2d%2dZ_%*dZ_N_O_%*dZ.nat%n", &sat_num, &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min,
                                &timeS.tm_sec, &len) == 7 &&
                         len == f->name.size())
                     {
                         i.type = NATIVE_METOP_AMSUA;

                         timeS.tm_year -= 1900;
                         timeS.tm_mon -= 1;
                         i.timestamp = timegm(&timeS);

                         i.name = MetOpNumToName(sat_num) + " AMSU-A " + timestamp_to_string(i.timestamp);
                     }

                     return i;
                 },
                 []() { return std::make_shared<AMSUNatProcessor>(); }},

                {NATIVE_METOP_HIRS,
                 [](std::shared_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     FirstPartyProductInfo i;

                     int sat_num, len;
                     std::tm timeS;
                     memset(&timeS, 0, sizeof(std::tm));
                     if (sscanf(f->name.c_str(), "HIRS_xxx_1B_M%2d_%4d%2d%2d%2d%2d%2dZ_%*dZ_N_O_%*dZ.nat%n", &sat_num, &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min,
                                &timeS.tm_sec, &len) == 7 &&
                         len == f->name.size())
                     {
                         i.type = NATIVE_METOP_HIRS;

                         timeS.tm_year -= 1900;
                         timeS.tm_mon -= 1;
                         i.timestamp = timegm(&timeS);

                         i.name = MetOpNumToName(sat_num) + " HIRS " + timestamp_to_string(i.timestamp);
                     }

                     return i;
                 },
                 []() { return std::make_shared<HIRSNatProcessor>(); }},

                {NATIVE_METOP_IASI,
                 [](std::shared_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     FirstPartyProductInfo i;

                     int sat_num, len;
                     std::tm timeS;
                     memset(&timeS, 0, sizeof(std::tm));
                     if (sscanf(f->name.c_str(), "IASI_xxx_1C_M%2d_%4d%2d%2d%2d%2d%2dZ_%*dZ_N_O_%*dZ.nat%n", &sat_num, &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min,
                                &timeS.tm_sec, &len) == 7 &&
                         len == f->name.size())
                     {
                         i.type = NATIVE_METOP_IASI;

                         timeS.tm_year -= 1900;
                         timeS.tm_mon -= 1;
                         i.timestamp = timegm(&timeS);

                         i.name = MetOpNumToName(sat_num) + " IASI " + timestamp_to_string(i.timestamp);
                     }

                     return i;
                 },
                 []() { return std::make_shared<IASINatProcessor>(); }},

                {NETCDF_AWS_MWR,
                 [](std::shared_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     FirstPartyProductInfo i;

                     int sat_num, len;
                     std::tm timeS;
                     memset(&timeS, 0, sizeof(std::tm));
                     uint32_t test1, test2;
                     int day_repeat_cycle;
                     if (sscanf(f->name.c_str(), "W_NO-KSAT-Tromso,SAT,AWS%1d-MWR-1B-RAD_C_OHB__%4d%2d%2d%2d%2d%2d_G_O_%14u_%14u_C_N____.nc%n", &sat_num, &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday,
                                &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec, &test1, &test2, &len) == 9 &&
                         len == f->name.size())
                     {
                         i.type = NETCDF_AWS_MWR;

                         timeS.tm_year -= 1900;
                         timeS.tm_mon -= 1;
                         i.timestamp = timegm(&timeS);

                         i.name = "AWS-" + std::to_string(sat_num) + " MWR " + timestamp_to_string(i.timestamp);

                         i.group_id = std::to_string(sat_num) + "_" + std::to_string((uint64_t)i.timestamp);
                     }

                     return i;
                 },
                 []() { return std::make_shared<MWRNcProcessor>(); }},

                {NETCDF_MTG_FCI,
                 [](std::shared_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     FirstPartyProductInfo i;

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

                         i.group_id = std::to_string(sat_num) + "_" + std::to_string((uint64_t)floor(i.timestamp / (24 * 3600))) + "_" + std::to_string(day_repeat_cycle);
                     }
                     else if (sscanf(f->name.c_str(), "W_XX-EUMETSAT-Darmstadt,IMG+SAT,MTI%d+FCI-1C-RRAD-HRFI-FD--CHK-BODY---NC4E_C_EUMT_%4d%2d%2d%2d%2d%2d_IDPFI_OPE_%14u_%14u_N__O_%4d_%*d.nc%n",
                                     &sat_num, &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min, &timeS.tm_sec, &test1, &test2, &day_repeat_cycle, &len) == 10 &&
                              len == f->name.size())
                     {
                         i.type = NETCDF_MTG_FCI;

                         timeS.tm_year -= 1900;
                         timeS.tm_mon -= 1;
                         i.timestamp = timegm(&timeS);

                         i.name = "MTG-I" + std::to_string(sat_num) + " FCI (Full Resolution) " + timestamp_to_string(i.timestamp);

                         i.group_id = std::to_string(sat_num) + "_fr_" + std::to_string((uint64_t)floor(i.timestamp / (24 * 3600))) + "_" + std::to_string(day_repeat_cycle);
                     }

                     return i;
                 },
                 []() { return std::make_shared<FCINcProcessor>(); }},

                {NETCDF_GOES_ABI,
                 [](std::shared_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     FirstPartyProductInfo i;

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

                         i.group_id = std::to_string(sat_num) + "_" + std::to_string((time_t)i.timestamp);
                     }

                     return i;
                 },
                 []() { return std::make_shared<ABINcProcessor>(); }},

                {HSD_HIMAWARI_AHI,
                 [](std::shared_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     FirstPartyProductInfo i;

                     int sat_num, len;
                     std::tm timeS;
                     memset(&timeS, 0, sizeof(std::tm));
                     char mode[5];
                     if (sscanf(f->name.c_str(), "HS_H%2d_%4d%2d%2d_%2d%2d_B%*d_%4s_R%*d_S%*d.DAT.bz2%n", &sat_num, &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min, mode,
                                &len) == 7 &&
                         len == f->name.size())
                     {
                         i.type = HSD_HIMAWARI_AHI;

                         timeS.tm_year -= 1900;
                         timeS.tm_mon -= 1;
                         i.timestamp = timegm(&timeS);

                         i.name = "Himawari-" + std::to_string(sat_num) + " AHI (" + std::string(mode) + ") " + timestamp_to_string(i.timestamp);

                         i.group_id = std::to_string(sat_num) + "_" + std::string(mode) + "_" + std::to_string((time_t)i.timestamp);
                     }

                     return i;
                 },
                 []() { return std::make_shared<AHIHsdProcessor>(); }},

                {NETCDF_GK2A_AMI,
                 [](std::shared_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     FirstPartyProductInfo i;

                     int len;
                     std::tm timeS;
                     memset(&timeS, 0, sizeof(std::tm));
                     char channel[6];
                     char mode[3];
                     char extra[6];
                     if (sscanf(f->name.c_str(), "gk2a_ami_le1b_%5s_%2s%5s_%4d%2d%2d%2d%2d.nc%n", channel, mode, extra, &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min,
                                &len) == 8 &&
                         len == f->name.size())
                     {
                         i.type = NETCDF_GK2A_AMI;

                         timeS.tm_year -= 1900;
                         timeS.tm_mon -= 1;
                         i.timestamp = timegm(&timeS);

                         if (std::string(mode) == "fd")
                             i.name = "GK-2A AMI (Full Disk) " + timestamp_to_string(i.timestamp);
                         else if (std::string(mode) == "la")
                             i.name = "GK-2A AMI (Local Area) " + timestamp_to_string(i.timestamp);
                         else
                             i.name = "GK-2A AMI " + timestamp_to_string(i.timestamp);

                         i.group_id = std::string(mode) + "_" + std::to_string((time_t)i.timestamp);
                     }

                     return i;
                 },
                 []() { return std::make_shared<AMINcProcessor>(); }},

                {HDF_FY2_IMAGER,
                 [](std::shared_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     FirstPartyProductInfo i;

                     char sat_name[2];
                     int len;
                     std::tm timeS;
                     memset(&timeS, 0, sizeof(std::tm));
                     if (sscanf(f->name.c_str(), "FY2%1s_FDI_ALL_NOM_%4d%2d%2d_%2d%2d.hdf%n", sat_name, &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min, &len) == 6 &&
                         len == f->name.size())
                     {
                         i.type = HDF_FY2_IMAGER;

                         timeS.tm_year -= 1900;
                         timeS.tm_mon -= 1;
                         i.timestamp = timegm(&timeS);

                         i.name = "FY-2" + std::string(sat_name) + " S-VISSR " + timestamp_to_string(i.timestamp);

                         i.group_id = std::string(sat_name) + "_" + std::to_string((time_t)i.timestamp);
                     }

                     return i;
                 },
                 []() { return std::make_shared<SVISSRHdfProcessor>(); }},

                {HDF_FY3_MERSI,
                 [](std::shared_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     FirstPartyProductInfo i;

                     char sat_name[2], dtype[5], ftype[6], fver[3];
                     int len;
                     std::tm timeS;
                     memset(&timeS, 0, sizeof(std::tm));
                     if (sscanf(f->name.c_str(), "FY3%1s_MERSI_%4s_L1_%4d%2d%2d_%2d%2d_%5s_%2s.HDF%n", sat_name, dtype, &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min,
                                ftype, fver, &len) == 9 &&
                         len == f->name.size())
                     {
                         i.type = HDF_FY3_MERSI;

                         timeS.tm_year -= 1900;
                         timeS.tm_mon -= 1;
                         i.timestamp = timegm(&timeS);

                         // TODOREWORK. FY-3A/B/C?
                         if (std::string(sat_name) == "D")
                             i.name = "FY-3" + std::string(sat_name) + " MERSI-2 " + timestamp_to_string(i.timestamp);
                         else if (std::string(sat_name) == "E")
                             i.name = "FY-3" + std::string(sat_name) + " MERSI-LL " + timestamp_to_string(i.timestamp);
                         else if (std::string(sat_name) == "F")
                             i.name = "FY-3" + std::string(sat_name) + " MERSI-3 " + timestamp_to_string(i.timestamp);
                         else if (std::string(sat_name) == "G")
                             i.name = "FY-3" + std::string(sat_name) + " MERSI-RM " + timestamp_to_string(i.timestamp);

                         i.group_id = std::string(dtype) + "_" + std::string(sat_name) + "_" + std::to_string((time_t)i.timestamp);
                     }

                     return i;
                 },
                 []() { return std::make_shared<MERSIHdfProcessor>(); }},

                {HDF_GPM_GMI,
                 [](std::shared_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     FirstPartyProductInfo i;

                     int unknown1, len;
                     std::tm timeS;
                     memset(&timeS, 0, sizeof(std::tm));
                     if (sscanf(f->name.c_str(), "1B.GPM.GMI.TB2021.%4d%2d%2d-S%2d%2d%2d-E%6d.V07B.RT-H5%n", &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min,
                                &timeS.tm_sec, &unknown1, &len) == 7 &&
                         len == f->name.size())
                     {
                         i.type = HDF_GPM_GMI;

                         timeS.tm_year -= 1900;
                         timeS.tm_mon -= 1;
                         i.timestamp = timegm(&timeS);

                         i.name = "GMP GMI " + timestamp_to_string(i.timestamp);

                         i.group_id = std::to_string((time_t)i.timestamp);
                     }

                     return i;
                 },
                 []() { return std::make_shared<GMIHdfProcessor>(); }},

                {HDF_DMSP_SSMIS,
                 [](std::shared_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     FirstPartyProductInfo i;

                     int sat, unknown1, len;
                     std::tm timeS;
                     memset(&timeS, 0, sizeof(std::tm));
                     if (sscanf(f->name.c_str(), "1C.F%2d.SSMIS.XCAL2021-V.%4d%2d%2d-S%2d%2d%2d-E%6d.V07B.RT-H5%n", &sat, &timeS.tm_year, &timeS.tm_mon, &timeS.tm_mday, &timeS.tm_hour, &timeS.tm_min,
                                &timeS.tm_sec, &unknown1, &len) == 8 &&
                         len == f->name.size())
                     {
                         i.type = HDF_DMSP_SSMIS;

                         timeS.tm_year -= 1900;
                         timeS.tm_mon -= 1;
                         i.timestamp = timegm(&timeS);

                         i.name = "DMSP-F" + std::to_string(sat) + " SSMIS " + timestamp_to_string(i.timestamp);

                         i.group_id = std::to_string(sat) + "_" + std::to_string((time_t)i.timestamp);
                     }

                     return i;
                 },
                 []() { return std::make_shared<SSMISHdfProcessor>(); }},

                {HRIT_GENERIC,
                 [](std::shared_ptr<satdump::utils::FilesIteratorItem> &f)
                 {
                     FirstPartyProductInfo i;

                     xrit::XRITFile file;
                     file.lrit_data = f->getPayload();
                     file.parseHeaders();

                     xrit::XRITFileInfo ii = xrit::identifyXRITFIle(file);

                     if (ii.type != xrit::XRIT_UNKNOWN)
                     {
                         i.type = HRIT_GENERIC;
                         i.timestamp = ii.timestamp;
                         i.name = ii.satellite_name + " " + ii.instrument_name + " " + timestamp_to_string(i.timestamp);
                         i.group_id = ii.groupid;
                     }

                     return i;
                 },
                 []() { return std::make_shared<HRITGenericProcessor>(); }},
            };
        }

        FirstPartyProductInfo parseFirstPartyInfo(std::shared_ptr<satdump::utils::FilesIteratorItem> &f)
        {
            for (auto &p : getRegisteredProducts())
            {
                auto finfo = p.testFile(f);
                if (finfo.type != PRODUCT_NONE)
                    return finfo;
            }
            return FirstPartyProductInfo();
        }

        std::shared_ptr<FirstPartyProductProcessor> getProcessorForProduct(FirstPartyProductInfo info)
        {
            for (auto &p : getRegisteredProducts())
            {
                if (p.type == info.type)
                    return p.getProcessor();
            }
            throw satdump_exception("No processor found for " + info.name + "!");
        }
    } // namespace firstparty
} // namespace satdump