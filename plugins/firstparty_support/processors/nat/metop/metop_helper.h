#pragma once

#include "common/tracking/tle.h"
#include "core/exception.h"
#include "init.h"
#include "libs/bzlib_utils.h"
#include "logger.h"
#include <cstdint>

namespace satdump
{
    namespace firstparty
    {
        struct MetOpSatInfo
        {
            std::string sat_name;
            int norad = 0;
            std::optional<satdump::TLE> satellite_tle;
        };

        inline MetOpSatInfo getMetOpSatInfoFromID(std::string sat_id, double prod_timestamp)
        {
            MetOpSatInfo info;

            info.sat_name = "Unknown MetOp";
            if (sat_id == "M02")
                info.sat_name = "MetOp-A";
            else if (sat_id == "M01")
                info.sat_name = "MetOp-B";
            else if (sat_id == "M03")
                info.sat_name = "MetOp-C";

            info.norad = 0;
            if (sat_id == "M02")
                info.norad = 29499;
            else if (sat_id == "M01")
                info.norad = 38771;
            else if (sat_id == "M03")
                info.norad = 43689;

            info.satellite_tle = satdump::db_tle->get_from_norad_time(info.norad, prod_timestamp);

            return info;
        }

        inline void tryDecompressBzip2(std::vector<uint8_t> &nat_file)
        {
            // TODDOREWORK Don't hardcode mazimum size!
            if (nat_file[0] == 0x42 && nat_file[1] == 0x5a && nat_file[2] == 0x68)
            {
                logger->info("MetOp Native file is bz2-compressed, decompressing...");

                size_t source_offset = 0, dest_offset = 0;
                const unsigned int buffer_size = 100000000;
                unsigned int dest_length;
                uint8_t *decomp_dest = new uint8_t[buffer_size];
                int bz2_result;

                // Decompress
                do
                {
                    dest_length = buffer_size - dest_offset;
                    unsigned int consumed;
                    bz2_result =
                        BZ2_bzBuffToBuffDecompress_M((char *)decomp_dest + dest_offset, &dest_length, (char *)nat_file.data() + source_offset, nat_file.size() - source_offset, &consumed, 0, 0);
                    source_offset += consumed;
                    dest_offset += dest_length;
                } while (bz2_result == 0 && nat_file.size() > source_offset);

                // Free up memory
                std::vector<uint8_t>().swap(nat_file);

                // Process if decompression was successful
                if (bz2_result != BZ_OK)
                {
                    delete[] decomp_dest;
                    throw satdump_exception("Could not decompress MetOp NAT!");
                }

                nat_file = std::vector<uint8_t>(decomp_dest, decomp_dest + dest_length);
                delete[] decomp_dest;
            }
        }
    } // namespace firstparty
} // namespace satdump